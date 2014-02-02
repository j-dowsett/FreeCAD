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

#include "PreCompiled.h"

#ifndef _PreComp_
//# include <boost/signals.hpp>
//# include <boost/bind.hpp>
//# include <QActionEvent>
//# include <QApplication>
//# include <QDesktopWidget>
//# include <QEvent>
# include <QMessageBox>
//# include <QTimer>
//# include <QToolBar>
//# include <QToolButton>
#endif

#include "Gui/Action.h"
#include "Gui/Application.h"
#include "Gui/Command.h"
//#include "Gui/DlgUndoRedo.h"
#include "Gui/FileDialog.h"
#include "Gui/MainWindow.h"
//#include "Gui/WhatsThis.h"
#include "Gui/Workbench.h"
//#include "Gui/WorkbenchManager.h"

//#include <Base/Exception.h>
//#include <App/Application.h>


#include "ActionTemplateMRU.h"
#include <Gui/BitmapFactory.h>

using namespace DrawingGui;


TemplatesRecentFilesAction::TemplatesRecentFilesAction ( Gui::Command* pcCmd, QObject * parent )
  : RecentFilesAction( pcCmd, parent )//, visibleItems(4), maximumItems(20)
{
    restore();
}

TemplatesRecentFilesAction::~TemplatesRecentFilesAction()
{
    save();
}


void TemplatesRecentFilesAction::setFiles(const QStringList& files)
{
    QList<QAction*> recentFiles = _group->actions();

    QRegExp rx(QString::fromAscii("(((?:A|B|C|D|E)\\d?)|(Legal|Ledger|Letter))_(Landscape|Portrait).*.svg"));

    int numRecentFiles = std::min<int>(recentFiles.count(), files.count());
    for (int index = 0; index < numRecentFiles; index++) {
        QFileInfo fi(files[index]);

        rx.indexIn(fi.fileName());

        QString paper = rx.cap(1);                                      // gives A4 / B / Legal etc
        QString twoLetters;
        if (!rx.cap(2).isEmpty())
            twoLetters = rx.cap(2);                                     // gives A4 / B etc
        else
            twoLetters = QString::fromAscii("L") + rx.cap(3).at(2);     // gives Lg / Ld etc
        QString orientation = rx.cap(4);

        QFile file(QString::fromAscii(":/icons/actions/drawing-%1-A0.svg").arg(orientation.toLower()));
        if (file.open(QFile::ReadOnly))
        {
            QString s = QString::fromAscii("style=\"font-size:22px\">%1</tspan></text>").arg(twoLetters);
            QByteArray data = file.readAll();
            data.replace("style=\"font-size:22px\">A0</tspan></text>", s.toAscii());
            recentFiles[index]->setIcon(Gui::BitmapFactory().pixmapFromSvg(data, QSize(24,24)));
        }

        recentFiles[index]->setText(QString::fromAscii("&%1 %2").arg(index+1)
            .arg(QCoreApplication::translate("Drawing_NewPage", "%1 %2", 0,QCoreApplication::CodecForTr)
                .arg(paper)
                .arg(orientation)));

        recentFiles[index]->setStatusTip(tr("Insert drawing based on template %1").arg(files[index]));
        recentFiles[index]->setToolTip(files[index]); // set the full name that we need later for saving
        recentFiles[index]->setData(QVariant(index));
        recentFiles[index]->setVisible(true);
    }

    // if less file names than actions
    numRecentFiles = std::min<int>(numRecentFiles, this->visibleItems);
    for (int index = numRecentFiles; index < recentFiles.count(); index++) {
        recentFiles[index]->setVisible(false);
        recentFiles[index]->setText(QString());
        recentFiles[index]->setToolTip(QString());
    }


/*
            a->setProperty("TemplatePaper", paper);
            a->setProperty("TemplateOrientation", orientation);
            a->setProperty("Template", dir.absoluteFilePath(dir[i]));
*/
}


void TemplatesRecentFilesAction::activateFile(int id)
{
    // restore the list of recent files
    App::Document * doc = App::GetApplication().getActiveDocument();

    QStringList files = this->files();
    if (id < 0 || id >= files.count())
        return; // no valid item

    QString filename = files[id];
    QFileInfo tfi(filename);
    if (tfi.isReadable())
    {
        std::string FeatName = doc->getUniqueObjectName("Page");
        appendFile(filename);
        Gui::Command::openCommand("Drawing create page");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('Drawing::FeaturePage','%s')",FeatName.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Template = '%s'",FeatName.c_str(), (const char*)tfi.filePath().toUtf8());
        Gui::Command::commitCommand();

        Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Drawing");
        hGrp->SetASCII("Last template", filename.toStdString().c_str());
    }
    else
    {
        QMessageBox::critical(Gui::getMainWindow(),
            QLatin1String("No template"),
            QLatin1String("No template available for this page size"));
    }

        /*
        // invokes appendFile()
        SelectModule::Dict dict = SelectModule::importHandler(filename);
        for (SelectModule::Dict::iterator it = dict.begin(); it != dict.end(); ++it) {
            Application::Instance->open(it.key().toUtf8(), it.value().toAscii());
            break;
        }
        */
}


void TemplatesRecentFilesAction::restore()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Drawing");

    //ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences");

    if (hGrp->HasGroup("Recent templates")) {
        hGrp = hGrp->GetGroup("Recent templates");
        // we want at least 20 items but we do only show the number of files
        // that is defined in user parameters
        this->visibleItems = hGrp->GetInt("Recent templates", this->visibleItems);
    }

    int count = std::max<int>(this->maximumItems, this->visibleItems);
    for (int i=0; i<count; i++)
        _group->addAction(QLatin1String(""))->setVisible(false);
    std::vector<std::string> MRU = hGrp->GetASCIIs("MRU");
    QStringList files;
    for (std::vector<std::string>::iterator it = MRU.begin(); it!=MRU.end();++it)
        files.append(QString::fromUtf8(it->c_str()));
    setFiles(files);
}


void TemplatesRecentFilesAction::save()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Drawing")->GetGroup("Recent templates");

//    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
//                                ->GetGroup("Preferences")->GetGroup("RecentFiles");
    int count = hGrp->GetInt("Recent templates", this->visibleItems); // save number of files
    hGrp->Clear();
    hGrp->SetInt("Recent templates", count); // restore

    // count all set items
    QList<QAction*> recentFiles = _group->actions();
    int num = std::min<int>(count, recentFiles.count());
    for (int index = 0; index < num; index++) {
        QString key = QString::fromAscii("MRU%1").arg(index);
        QString value = recentFiles[index]->toolTip();
        if (value.isEmpty())
            break;
        hGrp->SetASCII(key.toAscii(), value.toUtf8());
    }
}

#include "moc_ActionTemplateMRU.cpp"
