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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QtWidgets/QDialog>
#include <QtWidgets/QComboBox>

namespace Ui {
    class Settings;
}

/*!
 * The Settings GUI dialog box for configuring write options
 */
class Settings : public QDialog {
	// Some Qt macros
    Q_OBJECT
    Q_DISABLE_COPY(Settings)

public:
	// Methods
    explicit Settings(QWidget *parent = 0);
    virtual ~Settings();

    void enableEepromBox(bool Eeprom);
    void setWriteFlash(bool value);
    void setWriteEeprom(bool value);    

    bool writeFlash;
    bool writeEeprom;
    bool writeConfig;

    bool EepromPresent;
    bool hasConfig;

protected:
    virtual void changeEvent(QEvent* e);
    void populateBaudRates(QComboBox* comboBox);

private:
	Ui::Settings* ui;    

private slots:    
    void on_buttonBox_accepted();
};

#endif // SETTINGS_H
