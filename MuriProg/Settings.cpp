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

#include "Settings.h"
#include "ui_Settings.h"

#include <QtWidgets/QMessageBox>
Settings::Settings(QWidget *parent) : QDialog(parent), ui(new Ui::Settings)
{
    ui->setupUi(this);
}

Settings::~Settings()
{
    delete ui;
}

void Settings::enableEepromBox(bool Eeprom)
{
    ui->EepromCheckBox->setEnabled(Eeprom);
    EepromPresent = Eeprom;
}

void Settings::setWriteFlash(bool value)
{
    writeFlash = value;
    ui->FlashProgramMemorycheckBox->setChecked(value);
}

void Settings::setWriteEeprom(bool value)
{
    writeEeprom = value && EepromPresent;
    ui->EepromCheckBox->setChecked(value && EepromPresent);
}

void Settings::changeEvent(QEvent *e)
{
    switch (e->type())
    {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

void Settings::on_buttonBox_accepted()
{
    writeFlash = ui->FlashProgramMemorycheckBox->isChecked();    
    writeEeprom = ui->EepromCheckBox->isChecked();
}
