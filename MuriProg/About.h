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

#ifndef ABOUT_H
#define ABOUT_H

#include <QtWidgets\QDialog>

// This dialog simply shows a window for the user to read
// the MuriProg about information.

namespace Ui {
	class About;
}

class About : public QDialog {
	// Some Qt Macros
	Q_OBJECT
	Q_DISABLE_COPY(About);

public:
	explicit About(QWidget *parent = 0);
	virtual ~About();

private:
	Ui::About* ui;
};

#endif