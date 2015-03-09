/*
 * Program: Muriprog
 * Author: Michael D. Stone (AKA Neoaikon)
 * Date: 8/10/2014
 * Copyright (C) 2014 Mid-Ohio Area Robotics
 *
 * Description: Allows you to load a program onto
 * the Muribot robotic platform. This program is
 * heavily based on the HID bootloader example
 * distributed in Microchip Libraries for Applications
 * v2013-12-20.
 *
 * GPL V3 License
 * -------------
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

#include <QtWidgets/QApplication>
#include "MuriProg.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	// Setup organization, domain, and application.
    QCoreApplication::setOrganizationName("Mid-Ohio Area Robotics");
    QCoreApplication::setOrganizationDomain("moarobotics.com");
    QCoreApplication::setApplicationName("MuriProg");

	// Create the window
    MuriProg w;
    w.show();
    return a.exec();
}
