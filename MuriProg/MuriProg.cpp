/*
 * This file is part of MuriProg.
 *
 * MuriProg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MuriProg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with MuriProg.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
NOTES -
	For the Muribot we use a variant of the Microchip HID Bootloader example. It has a few
	key differences though. First we no longer retrieve the extended firmware info, we know
	exactly what those values would be, so there's no use in retrieving them. Second, we
	send a command when we connect that tells the bootloader the PC software wishes to 
	connect to it. The Microchip example has a hardware jumper to enter into the bootloader,
	and we didn't want to have to make the user do anything special besides start the PC software.
*/

#include <QTextStream>
#include <QByteArray>
#include <QList>
#include <QTime>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QSettings>
#include <QtWidgets/QDesktopWidget>
#include <QtConcurrent/QtConcurrentRun>
#include <sstream>

#include "MuriProg.h"
#include "ui_MuriProg.h"

#include "Settings.h"
#include "About.h"

#include "../version.h"

//Surely the micro doesn't have a programmable memory region greater than 268 Megabytes...
//Value used for error checking device reponse values.
#define MAXIMUM_PROGRAMMABLE_MEMORY_SEGMENT_SIZE 0x0FFFFFFF

// Information about the firmware on the Muribot
USB::FirmwareInfo firmwareInfo;


MuriProg::MuriProg(QWidget *parent) : QMainWindow(parent), ui(new Ui::MuriProg)
{
    int i;
    hexOpen = false;
    fileWatcher = NULL;
    timer = new QTimer();

	// Window setup
    ui->setupUi(this);
    setWindowTitle(APPLICATION + QString(" v") + VERSION);

	// Fetch the saved settings
    QSettings settings;
    settings.beginGroup("MuriProg");
    fileName = settings.value("fileName").toString();
    for(i = 0; i < MAX_RECENT_FILES; i++)
    {
        recentFiles[i] = new QAction(this);
        connect(recentFiles[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
        recentFiles[i]->setVisible(false);
		ui->FileMenu->insertAction(ui->ExitAction, recentFiles[i]);
    }
    ui->FileMenu->insertSeparator(ui->ExitAction);
    settings.endGroup();

    settings.beginGroup("WriteOptions");
    writeFlash = settings.value("writeFlash", true).toBool();
    writeEeprom = settings.value("writeEeprom", false).toBool();
    eraseDuringWrite = true;
    settings.endGroup();

    comm = new USB();
    picData = new PICData();
    hexData = new PICData();
    device = new Bootloader(picData);

    qRegisterMetaType<USB::ErrorCode>("USB::ErrorCode");

	// Connect signals to their methods
	connect(timer, SIGNAL(timeout()), this, SLOT(Connection()));
    connect(this, SIGNAL(IoWithDeviceCompleted(QString,USB::ErrorCode,double)), this, SLOT(IoWithDeviceComplete(QString,USB::ErrorCode,double)));
    connect(this, SIGNAL(IoWithDeviceStarted(QString)), this, SLOT(IoWithDeviceStart(QString)));
    connect(this, SIGNAL(AppendString(QString)), this, SLOT(AppendStringToTextbox(QString)));
    connect(this, SIGNAL(SetProgressBar(int)), this, SLOT(UpdateProgressBar(int)));
    connect(comm, SIGNAL(SetProgressBar(int)), this, SLOT(UpdateProgressBar(int)));
	connect(ui->AboutAction, SIGNAL(triggered()), this, SLOT(About_Clicked()));
	connect(ui->EraseAction, SIGNAL(triggered()), this, SLOT(Erase_Clicked()));
	connect(ui->ExitAction, SIGNAL(triggered()), this, SLOT(Exit_Clicked()));
	connect(ui->OpenAction, SIGNAL(triggered()), this, SLOT(Open_Clicked()));
	connect(ui->ResetAction, SIGNAL(triggered()), this, SLOT(Reset_Clicked()));
	connect(ui->WriteAction, SIGNAL(triggered()), this, SLOT(Write_Clicked()));
	connect(ui->SettingsAction, SIGNAL(triggered()), this, SLOT(Settings_Clicked()));

	//Update the file list in the File-->[import files list] area, so the user can quickly re-load a previously used .hex file.
    UpdateRecentFileList();

    this->statusBar()->addPermanentWidget(&deviceLabel);
    deviceLabel.setText("Connecting...");

    // Make initial check to see if the USB device is attached
    comm->PollUSB();
    if(comm->isConnected())
    {
        qWarning("Attempting to open USB connection...");
        comm->open();
        ui->Output->setPlainText("Muribot detected!");
        ui->Output->appendPlainText("Attempting to connect...");		
		comm->EngageBootloader();
        GetFirmwareInfo();
    }
    else
    {
        ui->Output->appendPlainText("Muribot not detected...");
		ui->Output->appendPlainText("Verify that the USB cable is plugged in and the robot is turned on.");
        deviceLabel.setText("Disconnected");
        hexOpen = false;
        setBootloadEnabled(false);
        emit SetProgressBar(0);
    }

    timer->start(1000); //Check for future USB connection status changes every 1000 milliseconds.
}

MuriProg::~MuriProg()
{
	// Save the settings
    QSettings settings;
    settings.beginGroup("MuriProg");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.setValue("fileName", fileName);
    settings.endGroup();

    settings.beginGroup("WriteOptions");
    settings.setValue("writeFlash", writeFlash);
    settings.setValue("writeEeprom", writeEeprom);
    settings.endGroup();

	// Close the device and disable UI elements
    comm->close();
    setBootloadEnabled(false);

	// Free memory
    delete timer;
    delete ui;
    delete comm;
    delete picData;
    delete hexData;
    delete device;
}

/*
 * Poll for changes in the connection state
 */
void MuriProg::Connection(void)
{
    bool currStatus = comm->isConnected();
    USB::ErrorCode result;

    comm->PollUSB();	
	// If our state has changed...
    if(currStatus != comm->isConnected())
    {
        UpdateRecentFileList();
		// If we're connected, open that connection and notify the onboard bootloader
		// We want to load a program
        if(comm->isConnected())
        {
            qWarning("Attempting to open USB connection...");
            comm->open();
            ui->Output->setPlainText("Muribot detected!");
            ui->Output->appendPlainText("Attempting to connect...");
			/* BUGFIX: 3/8/2015 - Added the engage bootloader sequence here
				So this is why the bootloader wouldn't get engaged when we'd start the software
				after turning on the Muribot!
			*/
			comm->EngageBootloader();
            GetFirmwareInfo();
        }
        else // Otherwise close the device
        {
            qWarning("Closing device.");
            comm->close();
            deviceLabel.setText("Disconnected");
            ui->Output->setPlainText("Muribot detached.");
            hexOpen = false;
            setBootloadEnabled(false);
            emit SetProgressBar(0);
        }
    }
}

/*
 * Enable/Disable controls
 */
void MuriProg::setBootloadEnabled(bool enable)
{
	ui->SettingsAction->setEnabled(enable);    
    ui->WriteAction->setEnabled(enable && hexOpen);
    ui->ExitAction->setEnabled(enable);    
    ui->OpenAction->setEnabled(enable);    
    ui->ResetAction->setEnabled(enable);
	ui->EraseAction->setEnabled(enable);
}

/* 
 * Enable/Disable controls based on what the bootloader
 * is doing.
 */
void MuriProg::setBootloadBusy(bool busy)
{
    if(busy)
    {
        QApplication::setOverrideCursor(Qt::BusyCursor);
        timer->stop();
    }
    else
    {
        QApplication::restoreOverrideCursor();
        timer->start(1000);
    }

    ui->SettingsAction->setEnabled(!busy);
    ui->WriteAction->setEnabled(!busy && hexOpen);
    ui->ExitAction->setEnabled(!busy);    
    ui->OpenAction->setEnabled(!busy);
    ui->SettingsAction->setEnabled(!busy);    
    ui->ResetAction->setEnabled(!busy);
	ui->EraseAction->setEnabled(!busy);
}

/*
 * Self Explanitory...I mean...this really should be...
 */
void MuriProg::Exit_Clicked()
{
    QApplication::exit();
}

/*
 * Useful for setting the bootloader busy from other threads
 */ 
void MuriProg::IoWithDeviceStart(QString msg)
{
    ui->Output->appendPlainText(msg);
    setBootloadBusy(true);
}

/*
 * Useful for adding lines of text from other threads.
 */
void MuriProg::AppendStringToTextbox(QString msg)
{
    ui->Output->appendPlainText(msg);
}

/*
 *Useful for updating the progress bar from other threads
 */
void MuriProg::UpdateProgressBar(int newValue)
{
    ui->ProgressBar->setValue(newValue);
}

/*
 * Useful for indicating that the bootloader is no longer busy
 * and return a result message from other threads.
 */
void MuriProg::IoWithDeviceComplete(QString msg, USB::ErrorCode result, double time)
{
    QTextStream ss(&msg);

    switch(result)
    {
        case USB::Success:
            ss << " Complete (" << time << "s)\n";
            setBootloadBusy(false);
            break;
        case USB::NotConnected:
            ss << " Failed. Muribot not connected.\n";
            setBootloadBusy(false);
            break;
        case USB::Fail:
            ss << " Failed.\n";
            setBootloadBusy(false);
            break;
        case USB::IncorrectCommand:
            ss << " Failed. Unable to communicate with Firmware.\n";
            setBootloadBusy(false);
            break;
        case USB::Timeout:
            ss << " Timed out waiting for response (" << time << "s)\n";
            setBootloadBusy(false);
            break;
        default:
            break;
    }

    ui->Output->appendPlainText(msg);
}

/*
 * Routine that verifies the contents memory regions after programming.
 * This function requests the memory contents of the device, then
 * compares it against the parsed .hex file data to make sure the
 * locations that got programmed properly match.
 */
void MuriProg::VerifyDevice()
{
    USB::ErrorCode result;
    PICData::MemoryRange deviceRange, hexRange;
    QTime elapsed;
    unsigned int i, j;
    unsigned int arrayIndex;
    bool failureDetected = false;
    unsigned char flashData[MAX_ERASE_BLOCK_SIZE];
    unsigned char hexEraseBlockData[MAX_ERASE_BLOCK_SIZE];
    uint32_t startOfEraseBlock;
    uint32_t errorAddress = 0;
    uint16_t expectedResult = 0;
    uint16_t actualResult = 0;

    //Initialize an erase block sized buffer with 0xFF.
    //Used later for post SIGN_FLASH verify operation.
    memset(&hexEraseBlockData[0], 0xFF, MAX_ERASE_BLOCK_SIZE);

    emit IoWithDeviceStarted("Verifying memory...");
    foreach(deviceRange, picData->ranges)
    {
        if(writeFlash && (deviceRange.type == PROGRAM_MEM))
        {
            elapsed.start();

            //result = comm->GetData(deviceRange.start, device->bytesPerPacket, device->bytesPerAddressFLASH, device->bytesPerWordFLASH, deviceRange.end, deviceRange.pDataBuffer);
			result = comm->GetData(deviceRange.start, device->bytesPerPacket, 1, 2, deviceRange.end, deviceRange.pDataBuffer);

            if(result != USB::Success)
            {
                failureDetected = true;
                qWarning("Error reading memory.");
            }

            //Search through all of the programmable memory regions from the parsed .hex file data.
            //For each of the programmable memory regions found, if the region also overlaps a region
            //that was included in the device programmed area (which just got read back with GetData()),
            //then verify both the parsed hex contents and read back data match.
            foreach(hexRange, hexData->ranges)
            {
                if(deviceRange.start == hexRange.start)
                {
                    //For this entire programmable memory address range, check to see if the data read from the device exactly
                    //matches what was in the hex file.
                    for(i = deviceRange.start; i < deviceRange.end; i++)
                    {
                        //For each byte of each device address (1 on PIC18, 2 on PIC24, since flash memory is 16-bit WORD array)
                        for(j = 0; j < device->bytesPerAddressFLASH; j++)
                        {
                            //Check if the device response data matches the data we parsed from the original input .hex file.
                            if(deviceRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j] != hexRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j])
                            {
                                failureDetected = true;
                                qWarning("Memory: 0x%x Hex: 0x%x", deviceRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j], hexRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j]);
                                qWarning("Failed to verify Program Memory at address 0x%x", i);
                                emit IoWithDeviceCompleted("Verify", USB::Fail, ((double)elapsed.elapsed()) / 1000);
                                return;
                            }
                        }
                    }
                }
            }
        }
		else if(writeEeprom && (deviceRange.type == EEPROM_MEM))
        {
            elapsed.start();

            result = comm->GetData(deviceRange.start, device->bytesPerPacket, device->bytesPerAddressEEPROM, device->bytesPerWordEEPROM, deviceRange.end, deviceRange.pDataBuffer);

            if(result != USB::Success)
            {
                failureDetected = true;
                qWarning("Error reading memory.");
            }

            //Search through all of the programmable memory regions from the parsed .hex file data.
            //For each of the programmable memory regions found, if the region also overlaps a region
            //that was included in the device programmed area (which just got read back with GetData()),
            //then verify both the parsed hex contents and read back data match.
            foreach(hexRange, hexData->ranges)
            {
                if(deviceRange.start == hexRange.start)
                {
                    //For this entire programmable memory address range, check to see if the data read from the device exactly
                    //matches what was in the hex file.
                    for(i = deviceRange.start; i < deviceRange.end; i++)
                    {
                        //For each byte of each device address (only 1 for EEPROM byte arrays, presumably 2 for EEPROM WORD arrays)
                        for(j = 0; j < device->bytesPerAddressEEPROM; j++)
                        {
                            //Check if the device response data matches the data we parsed from the original input .hex file.
                            if(deviceRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressEEPROM)+j] != hexRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressEEPROM)+j])
                            {
                                //A mismatch was detected.
                                failureDetected = true;
                                qWarning("Device: 0x%x Hex: 0x%x", deviceRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j], hexRange.pDataBuffer[((i - deviceRange.start) * device->bytesPerAddressFLASH)+j]);
                                qWarning("Failed to verify EEPROM at address 0x%x", i);
                                emit IoWithDeviceCompleted("Verify EEPROM Memory", USB::Fail, ((double)elapsed.elapsed()) / 1000);
                                return;
                            }
                        }
                    }
                }
            }
        }
        else continue;
    }

    if(failureDetected == false)
    {
        //Successfully verified all regions without error.
        //If this is a v1.01 or later device, we now need to issue the SIGN_FLASH
        //command, and then re-verify the first erase page worth of flash memory
        //(but with the exclusion of the signature WORD address from the verify,
        //since the bootloader firmware will have changed it to the new/magic
        //value (probably 0x600D, or "good" in leet speak).        
        comm->SignFlash();

        qDebug("Expected Signature Address: 0x%x", firmwareInfo.signatureAddress);
        qDebug("Expected Signature Value: 0x%x", firmwareInfo.signatureValue);

		//Now re-verify the first erase page of flash memory.
        startOfEraseBlock = firmwareInfo.signatureAddress - (firmwareInfo.signatureAddress % firmwareInfo.erasePageSize);
        result = comm->GetData(startOfEraseBlock, device->bytesPerPacket, device->bytesPerAddressFLASH, device->bytesPerWordFLASH, (startOfEraseBlock + firmwareInfo.erasePageSize), &flashData[0]);
        if(result != USB::Success)
        {
			failureDetected = true;
            qWarning("Error reading, post signing, flash data block.");
        }

        //Search through all of the programmable memory regions from the parsed .hex file data.
        //For each of the programmable memory regions found, if the region also overlaps a region
        //that is part of the erase block, copy out bytes into the hexEraseBlockData[] buffer,
        //for re-verification.
        foreach(hexRange, hexData->ranges)
        {
            //Check if any portion of the range is within the erase block of interest in the device.
            if((hexRange.start <= startOfEraseBlock) && (hexRange.end > startOfEraseBlock))
            {
                unsigned int rangeSize = hexRange.end - hexRange.start;
                unsigned int address = hexRange.start;
                unsigned int k = 0;

                //Check every byte in the hex file range, to see if it is inside the erase block of interest
                for(i = 0; i < rangeSize; i++)
                {
                    //Check if the current byte we are looking at is inside the erase block of interst
                    if(((address+i) >= startOfEraseBlock) && ((address+i) < (startOfEraseBlock + firmwareInfo.erasePageSize)))
                    {
                        //The byte is in the erase block of interst.  Copy it out into a new buffer.
                        hexEraseBlockData[k] = *(hexRange.pDataBuffer + i);
                        //Check if this is a signature byte.  If so, replace the value in the buffer
                        //with the post-signing expected signature value, since this is now the expected
                        //value from the device, rather than the value from the hex file...
                        if((address+i) == firmwareInfo.signatureAddress)
                        {
                            hexEraseBlockData[k] = (unsigned char)firmwareInfo.signatureValue;    //Write LSB of signature into buffer
                        }
                        if((address+i) == (firmwareInfo.signatureAddress + 1))
                        {
                            hexEraseBlockData[k] = (unsigned char)(firmwareInfo.signatureValue >> 8); //Write MSB into buffer
                        }
                        k++;
                    }
                    if((k >= firmwareInfo.erasePageSize) || (k >= sizeof(hexEraseBlockData)))
                        break;
                }
            }
        }

        //We now have both the hex data and the post signing flash erase block data
        //in two RAM buffers.  Compare them to each other to perform post-signing
        //verify.
        for(i = 0; i < firmwareInfo.erasePageSize; i++)
        {
            if(flashData[i] != hexEraseBlockData[i])
            {
                failureDetected = true;
                qWarning("Post signing verify failure.");
                EraseDevice();  //Send an erase command, to forcibly
                //remove the signature (which might be valid), since
                //there was a verify error and we can't trust the application
                //firmware image integrity.  This ensures the device jumps
                //back into bootloader mode always.

                errorAddress = startOfEraseBlock + i;
                expectedResult = hexEraseBlockData[i] + ((uint32_t)hexEraseBlockData[i+1] << 8);
                actualResult = flashData[i] + ((uint32_t)flashData[i+1] << 8);
                break;
            }
        }
    }
    

    if(failureDetected == true)
    {
        qDebug("Verify failed at address: 0x%x", errorAddress);
        qDebug("Expected result: 0x%x", expectedResult);
        qDebug("Actual result: 0x%x", actualResult);
        emit AppendString("Operation aborted due to error encountered during verify stage.");
        emit AppendString("Please try programming the Muribot again.");
        emit AppendString("If repeated failures are encountered, this may indicate the flash");
        emit AppendString("memory has worn out, that the firmware has been damaged, or that");
        emit AppendString("there is some other unidentified problem.");
		emit AppendString("");
		emit AppendString("If you continue to experience problems, please contact Mid-Ohio Area Robotics");

        emit IoWithDeviceCompleted("Verify", USB::Fail, ((double)elapsed.elapsed()) / 1000);
    }
    else
    {
        emit IoWithDeviceCompleted("Verify", USB::Success, ((double)elapsed.elapsed()) / 1000);
        emit AppendString("Programming completed successfully!");
        emit AppendString("You may now turn off and unplug the Muribot.");
    }

    emit SetProgressBar(100);   //Set progress bar to 100%
}


/*
 * Start a thread to write the hexData to the device.
 */ 
void MuriProg::Write_Clicked()
{
    future = QtConcurrent::run(this, &MuriProg::WriteDevice);
    ui->Output->clear();
    ui->Output->appendPlainText("Attempting to program the Muribot");
    ui->Output->appendPlainText("Do not unplug it or turn it off until the operation is fully complete.");
    ui->Output->appendPlainText(" ");
}


// Writes the parsed file memory ranges contained in hexData->ranges to the
// Muribot.
void MuriProg::WriteDevice(void)
{
    QTime elapsed;
    USB::ErrorCode result;
    PICData::MemoryRange hexRange;

    //Update the progress bar so the user knows things are happening.
    emit SetProgressBar(3);
    //First erase the entire device.
    EraseDevice();

    //Now being re-programming each section based on the info we obtained when
    //we parsed the user's .hex file.

    emit IoWithDeviceStarted("Writing memory...");
    foreach(hexRange, hexData->ranges)
    {
        if(writeFlash && (hexRange.type == PROGRAM_MEM))
        {
            elapsed.start();

            result = comm->Program(hexRange.start,
                                   device->bytesPerPacket,
                                   device->bytesPerAddressFLASH,
                                   device->bytesPerWordFLASH,
                                   hexRange.end,
                                   hexRange.pDataBuffer);
        }
        else if(writeEeprom && (hexRange.type ==  EEPROM_MEM))
        {
                elapsed.start();

                result = comm->Program(hexRange.start,
                                       device->bytesPerPacket,
                                       device->bytesPerAddressEEPROM,
                                       device->bytesPerWordEEPROM,
                                       hexRange.end,
                                       hexRange.pDataBuffer);
        }
		else continue;

        if(result != USB::Success)
        {
            qWarning("Programming failed");
            return;
        }
    }

    emit IoWithDeviceCompleted("Write", result, ((double)elapsed.elapsed()) / 1000);

    VerifyDevice();
}

/*
 * Start a thread to erase the program memory
 */
void MuriProg::Erase_Clicked()
{
    future = QtConcurrent::run(this, &MuriProg::EraseDevice);
}

/*
 * Send an Erase command
 */
void MuriProg::EraseDevice(void)
{
    QTime elapsed;
    USB::ErrorCode result;    

    emit IoWithDeviceStarted("Erasing memory... (no status update until complete, may take several seconds)");
    elapsed.start();

    result = comm->Erase();
    if(result != USB::Success)
    {
        emit IoWithDeviceCompleted("Erase", result, ((double)elapsed.elapsed()) / 1000);
        return;
    }    

    emit IoWithDeviceCompleted("Erase", result, ((double)elapsed.elapsed()) / 1000);
}

/*
 * Display an open file dialog allowing the user to select a file.
 * Upon selecting the file, use LoadFile() to parse it.
 */
void MuriProg::Open_Clicked()
{
    QString msg, newFileName;
    QTextStream stream(&msg);

    //Create an open file dialog box, so the user can select a .hex file.
    newFileName =
        QFileDialog::getOpenFileName(this, "Open Hex File", fileName, "Hex Files (*.hex *.ehx)");

    if(newFileName.isEmpty())
    {
        return;
    }

    LoadFile(newFileName);
}

/*
 * Parse each line of the selected file into a memory range and
 * load those ranges into hexData->ranges;
 */
void MuriProg::LoadFile(QString newFileName)
{
    QString msg;
    QTextStream stream(&msg);
    QFileInfo nfi(newFileName);

    QApplication::setOverrideCursor(Qt::BusyCursor);

    HexLoader import;
    HexLoader::ErrorCode result;
    USB::ErrorCode commResultCode;

    hexData->ranges.clear();

    //Print some debug info to the debug window.
    qDebug(QString("Total programmable regions reported by Firmware: " + QString::number(picData->ranges.count(), 10)).toLatin1());

    //First duplicate the picData programmable region list and
    //allocate some RAM buffers to hold the hex data that we are about to import.
    foreach(PICData::MemoryRange range, picData->ranges)
    {
        //Allocate some RAM for the hex file data we are about to import.
        //Initialize all bytes of the buffer to 0xFF, the default unprogrammed memory value,
        //which is also the "assumed" value, if a value is missing inside the .hex file, but
        //is still included in a programmable memory region.
        range.pDataBuffer = new unsigned char[range.dataBufferLength];
        memset(range.pDataBuffer, 0xFF, range.dataBufferLength);
        hexData->ranges.append(range);

        //Print info regarding the programmable memory region to the debug window.
        qDebug(QString("Programmable memory region: [" + QString::number(range.start, 16).toUpper() + " - " +
                   QString::number(range.end, 16).toUpper() +")").toLatin1());
    }

    //Import the hex file data into the hexData->ranges[].pDataBuffer buffers.
    result = import.ImportHexFile(newFileName, hexData, device);
    //Based on the result of the hex file import operation, decide how to proceed.
    switch(result)
    {
        case HexLoader::Success:
            break;

        case HexLoader::CouldNotOpenFile:
            QApplication::restoreOverrideCursor();
            stream << "Error: Could not open file " << nfi.fileName() << "\n";
            ui->Output->appendPlainText(msg);
            return;

        case HexLoader::NoneInRange:
            QApplication::restoreOverrideCursor();
            stream << "No address within range in file: " << nfi.fileName() << ".  Verify the correct file was selected.\n";
            ui->Output->appendPlainText(msg);
            return;

        case HexLoader::ErrorInHexFile:
            QApplication::restoreOverrideCursor();
            stream << "Error in hex file.  Please make sure the correct file was selected.\n";
            ui->Output->appendPlainText(msg);
            return;
        case HexLoader::InsufficientMemory:
            QApplication::restoreOverrideCursor();
            stream << "Memory allocation failed.  Please close other applications to free up system RAM and try again. \n";
            ui->Output->appendPlainText(msg);
            return;

        default:
            QApplication::restoreOverrideCursor();
            stream << "Failed to import: " << result << "\n";
            ui->Output->appendPlainText(msg);
            return;
    }

    fileName = newFileName;
    watchFileName = newFileName;

    QSettings settings;
    settings.beginGroup("MuriProg");

    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while(files.size() > MAX_RECENT_FILES)
    {
        files.removeLast();
    }
    settings.setValue("recentFileList", files);
    UpdateRecentFileList();

    stream.setIntegerBase(10);

    msg.clear();
    QFileInfo fi(fileName);
    QString name = fi.fileName();
    stream << "Opened: " << name << "\n";
    ui->Output->appendPlainText(msg);
    hexOpen = true;
    setBootloadEnabled(true);
    QApplication::restoreOverrideCursor();

    return;
}

/*
 * Opens one of the files on the recently opened file list
 */
void MuriProg::openRecentFile(void)
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        LoadFile(action->data().toString());
    }
}

/*
 * Updates the recently opened file list
 */
void MuriProg::UpdateRecentFileList(void)
{
    QSettings settings;
    settings.beginGroup("MuriProg");
    QStringList files;

    files = settings.value("recentFileList").toStringList();

    int recentFileCount = qMin(files.size(), MAX_RECENT_FILES);
    QString text;
    int i;

    for(i = 0; i < recentFileCount; i++)
    {
        text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());

        recentFiles[i]->setText(text);
        recentFiles[i]->setData(files[i]);
        recentFiles[i]->setVisible(comm->isConnected());
    }

    for(; i < MAX_RECENT_FILES; i++)
    {
        recentFiles[i]->setVisible(false);
    }
}

/* 
 * Displays the About dialog
 */
void MuriProg::About_Clicked()
{
    USB::ErrorCode result;
    About* dlg = new About(this);

    dlg->exec();

    delete dlg;
}

/* 
 * Loads firmwareInfo with information about the Muribots bootloader
 * bootloader.
 */
void MuriProg::GetFirmwareInfo()
{
    QTime totalTime;
    PICData::MemoryRange range;
    QString connectMsg;
    QTextStream ss(&connectMsg);

    qDebug("Executing GetFirmwareInfo() command.");

    totalTime.start();

    if(!comm->isConnected())
    {
        qWarning("GetFirmwareInfo command not sent, Muribot not connected");
        return;
    }

    //Send the ReadFirmwareInfo command to the device over USB, and check the result status.
    switch(comm->ReadFirmwareInfo(&firmwareInfo))
    {
        case USB::Fail:
        case USB::IncorrectCommand:
            ui->Output->appendPlainText("Unable to communicate with firmware.\n");
            return;
        case USB::Timeout:
            ss << "Operation timed out";
            break;
		case USB::Success:			
			ss << "Connected to Muribot";
			deviceLabel.setText("Connected");
			break;
        default:
            return;
    }	
    ss << " (" << (double)totalTime.elapsed() / 1000 << "s)\n";
	ss << "Application Version: 0x" << QString::number(firmwareInfo.applicationVersion, 16) << "\n";
	ss << "Bootloader Version: 0x" << QString::number(firmwareInfo.bootloaderVersion, 16) << "\n";
	
    ui->Output->appendPlainText(connectMsg);    
		
    //Make sure user has allowed at least one region to be programmed
    if(!(writeFlash || writeEeprom))
    {
        setBootloadEnabled(false);
        ui->SettingsAction->setEnabled(true);
    }
    else
        setBootloadEnabled(true);
}

/*
 * Displays the settings dialog
 */
void MuriProg::Settings_Clicked()
{
    USB::ErrorCode result;
    Settings* dlg = new Settings(this);

    dlg->enableEepromBox(device->EepromPresent());

    dlg->setWriteFlash(writeFlash);    
    dlg->setWriteEeprom(writeEeprom);

    if(dlg->exec() == QDialog::Accepted)
    {
        writeFlash = dlg->writeFlash;
        writeEeprom = dlg->writeEeprom;
		
        if(!(writeFlash || writeEeprom))
        {
            setBootloadEnabled(false);
            ui->SettingsAction->setEnabled(true);
        }
        else
        {
            setBootloadEnabled(true);
        }
    }

    delete dlg;
}

/*
 *Reset the Muribot
 */
void MuriProg::Reset_Clicked()
{
    if(!comm->isConnected())
    {
        failed = -1;
        qWarning("Reset not sent, Muribot not connected");
        return;
    }

    ui->Output->appendPlainText("Resetting firmware...");
    comm->Reset();
}