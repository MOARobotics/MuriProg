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

#ifndef MURIPROG_H
#define MURIPROG_H

// Includes
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QtCore/QProcess>
#include <QtWidgets/QMenu>
#include <QFuture>

#include "USB.h"
#include "PICData.h"
#include "Bootloader.h"
#include "HexLoader.h"

namespace Ui
{
    class MuriProg;
}

// Maximum number of recent files to display in the 
#define MAX_RECENT_FILES 5

// The main Serial Bootloader GUI window.
class MuriProg : public QMainWindow
{
    Q_OBJECT

public:
	// Constructor/Destructor
    MuriProg(QWidget *parent = 0);
    ~MuriProg();

	// Methods
    void GetFirmwareInfo(void);
    void LoadFile(QString fileName);
    void EraseDevice(void);
    void BlankCheckDevice(void);
    void WriteDevice(void);
    void VerifyDevice(void);
    void setBootloadBusy(bool busy);

signals:
    void IoWithDeviceCompleted(QString msg, USB::ErrorCode, double time);
    void IoWithDeviceStarted(QString msg);
    void AppendString(QString msg);
    void SetProgressBar(int newValue);

public slots:
    void Connection(void);
    void openRecentFile(void);
    void IoWithDeviceComplete(QString msg, USB::ErrorCode, double time);
    void IoWithDeviceStart(QString msg);
    void AppendStringToTextbox(QString msg);
    void UpdateProgressBar(int newValue);

protected:
	// Members
    USB* comm;
    PICData* picData;
    PICData* hexData;
    Bootloader* device;
    QFuture<void> future;
    QString fileName, watchFileName;
    QFileSystemWatcher* fileWatcher;
    QTimer *timer;
    bool writeFlash;
    bool writeEeprom;    
    bool eraseDuringWrite;
    bool hexOpen;

	// Methods
    void setBootloadEnabled(bool enable);
    void UpdateRecentFileList(void);
    USB::ErrorCode RemapInterruptVectors(Bootloader* bootDevice, PICData* picData);

private:
	// Members
	Ui::MuriProg* ui;
    QLabel deviceLabel;
    int failed;
    QAction *recentFiles[MAX_RECENT_FILES];    

private slots:
	// Methods    
    void Reset_Clicked();
    void Settings_Clicked();    
    void About_Clicked();
    void Write_Clicked();
    void Open_Clicked();
    void Erase_Clicked();
    void Exit_Clicked();
};

#endif // MAINWINDOW_H
