/***************************************************************************
 *   Copyright (c) 2014 Joe Dowsett <dowsettjoe[at]yahoo[dot]co[dot]uk>    *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef ACTIONTEMPLATEMRU_H
#define ACTIONTEMPLATEMRU_H

#include "PreCompiled.h"
#include <QAction>
#include "Gui/Action.h"

class Command;
//class Gui::RecentFilesAction;

namespace DrawingGui
{
//class Gui::RecentFilesAction;

class TemplatesRecentFilesAction : public Gui::RecentFilesAction
{
    Q_OBJECT

public:
    TemplatesRecentFilesAction(Gui::Command* pcCmd, QObject * parent = 0);
    virtual ~TemplatesRecentFilesAction();

    void activateFile(int);


private:
    void setFiles(const QStringList& fileList);
    void restore();
    void save();
};

}
#endif
