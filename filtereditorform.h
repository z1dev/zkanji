/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef FILTEREDITORFORM_H
#define FILTEREDITORFORM_H

#include <QtCore>
#include <QMainWindow>
#include "dialogwindow.h"

namespace Ui {
    class FilterEditorForm;
}

struct WordDefAttrib;
// Form for editing word filter attributes, that can be accepted, applied, or the operation
// can be cancelled. The form can be shown with editWordFilter() found in dialogs.h. Setting
// the filter index to -1 will disable the apply button and create a new filter if accepted.
class FilterEditorForm : public DialogWindow
{
    Q_OBJECT
public:
    FilterEditorForm() = delete;
    FilterEditorForm(const FilterEditorForm&) = delete;
    FilterEditorForm(FilterEditorForm&&) = delete;

    virtual ~FilterEditorForm();
protected:
    virtual void closeEvent(QCloseEvent *e) override;
    virtual void changeEvent(QEvent *e) override;
protected slots:
    //// There's a global map holding all the filter editor forms. Only one/ form can be open
    //// for each filter. If A filter is erased, the map is updated here.
    //static void filterErased(int index);
private slots:
    void on_applyButton_clicked();
    void on_acceptButton_clicked();
    void on_cancelButton_clicked();
    void filterChanged(int index);
private:
    FilterEditorForm(int filterindex, QWidget *parent = nullptr);

    // Checks whether the name in the edit box has already been specified for another filter.
    // If msgbox is true, a message box is shown on error, when the name is empty or
    // conflicts with another filter. Returns false on error.
    bool checkName(bool msgbox = true);

    // Called on change to update the enabled state of the apply button.
    void allowApply();

    Ui::FilterEditorForm *ui;

    int filterindex;

    friend void editWordFilter(int, QWidget*);

    typedef DialogWindow    base;
};


#endif // FILTEREDITORFORM_H
