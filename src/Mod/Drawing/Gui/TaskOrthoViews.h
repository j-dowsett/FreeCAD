/***************************************************************************
 *   Copyright (c) 2011 Joe Dowsett <j-dowsett[at]users.sourceforge.net>   *
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

#ifndef GUI_TASKVIEW_TASKORTHOVIEWS_H
#define GUI_TASKVIEW_TASKORTHOVIEWS_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>
#include "ui_TaskOrthoViews.h"
#include <Base/BoundBox.h>

#include <gp_Ax2.hxx>
#include <vector>


class Ui_TaskOrthoViews;
using namespace std;

namespace DrawingGui {


class orthoView
{
public:
    orthoView(string name, string label, const char * targetpage, const char * sourcepart, Base::BoundBox3d * partbox);
    ~orthoView();

    void set_projection(gp_Ax2 cs);
    void setPos(float = 0, float = 0);
    void setScale(float);
    void deleteme();
    void hidden(bool);
    void smooth(bool);

private:
    void calcCentre();

private:
    string  myname;
    float   x, y;                   // 2D projection coords of bbox centre relative to origin
    float   cx, cy, cz;             // coords of bbox centre in 3D space
    float   pageX, pageY;           // required coords of projection centre on page
    float   scale;                  // scale of projection
    gp_Dir  X_dir, Y_dir, Z_dir;
};




struct view_struct
{
    bool        ortho;
    int         rel_x, rel_y;
    orthoView * view;
    bool        auto_scale;         // has scale for axo view been manually changed?
    gp_Dir      up, right;          // directions used to create axo
    bool        away, tri;          //  "   "
    int         axo;                // 0 / 1 / 2  =  iso / di / tri metric
    float       axo_scale;          // scale for axo view
};


class OrthoViews
{
public:
    OrthoViews(const char * pagename, const char * partname);
    ~OrthoViews();

    void    set_primary(gp_Dir facing, gp_Dir right);
    void    add_view(int rel_x, int rel_y);
    void    del_view(int rel_x, int rel_y);
    void    del_all();
    void    set_projection(int proj);
    void    set_hidden(bool state);
    void    set_smooth(bool state);
    void    set_Axo(int rel_x, int rel_y, gp_Dir up, gp_Dir right, bool away = false, int axo = 0, bool tri = false);
    void    set_Axo(int rel_x, int rel_y);
    void    set_Axo_scale(int rel_x, int rel_y, float axo_scale);
    void    set_Ortho(int rel_x, int rel_y);
    int     is_Ortho(int rel_x, int rel_y);
    bool    get_Axo(int rel_x, int rel_y, int & axo, gp_Dir & up, gp_Dir & right, bool & away, bool & tri, float & axo_scale);
    void    auto_dims(bool setting);
    void    set_configs(float configs[5]);
    void    get_configs(float configs[5]);

private:
    void    set_orientation(int index);
    void    set_all_orientations();
    void    calc_layout_size();
    void    calc_offsets();
    void    set_views();
    void    calc_scale();
    void    process_views();
    int     index(int rel_x, int rel_y);

private:
    vector<view_struct>     views;
    Base::BoundBox3d        bbox;
    App::Document *         parent_doc;

    string  page_name, part_name;
    int     size_x, size_y;                 // page working space (ie size inside of margins)
    int     rotate_coeff;                   // 1st (= -1) or 3rd (= 1) angle
    int     min_r_x, max_r_x;               // extreme relative positions of views
    int     min_r_y, max_r_y;               //      "       "       "
    float   width, height, depth;           // of non-scaled primary view
    float   layout_width, layout_height;    // of non-scaled layout without spaces
    float   gap_x, gap_y, min_space;        // required spacing between views
    float   offset_x, offset_y;             // coords of centre of upper left view
    float   scale;
    int     margin;
    int     num_gaps_x, num_gaps_y;         // how many gaps between views/edges? = num of views in given direction + 1
    gp_Ax2  primary;                        // coord system of primary view

    bool    hidden, smooth;
    bool    autodims;
};




class TaskOrthoViews : public QWidget//: public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskOrthoViews(QWidget *parent = 0);
    ~TaskOrthoViews();
    bool user_input();
    void clean_up(bool);

protected Q_SLOTS:
    void ShowContextMenu(const QPoint & pos);
    void setPrimary(int);
    void cb_toggled(bool);
    void projectionChanged(int);
    void hidden(int);
    void smooth(int);
    void toggle_auto(int);
    void data_entered();
    void change_axo(int p = 3);
    void axo_button();
    void axo_scale();

protected:
    void changeEvent(QEvent *);

private:
    void setup_axo_tab();
    void set_configs();

private:
    //class Private;
    Ui_TaskOrthoViews * ui;

    OrthoViews *    orthos;
    QCheckBox *     c_boxes[5][5];      // matrix of pointers to gui checkboxes
    QLineEdit *     inputs[5];          // pointers to manual position/scale boxes

    float   data[5];                    // pointers to scale, x_pos, y_pos, horiz, vert
    int     axo_r_x, axo_r_y;           // relative position of axo view currently being edited
    //int     num_axo;                    // how many axo views do we have
};


//////////////////////////////////////////////////////////////



/// simulation dialog for the TaskView
class TaskDlgOrthoViews : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgOrthoViews();
    ~TaskDlgOrthoViews();

public:
    void open();
    bool accept();
    bool reject();
    void clicked(int);

private:
    TaskOrthoViews * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace DrawingGui


#endif // GUI_TASKVIEW_OrthoViews_H

