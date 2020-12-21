#pragma once

#include <QMainWindow>
#include "qjsonmodel.h"

namespace Ui {
class JsonEditor;
}

class JsonEditor : public QMainWindow
{
    Q_OBJECT

public:
    static const int TIME_UPDATE_TABLE = 2000;  ///< 2 sec
    static const QMap<QJsonValue::Type, QString> TYPES_STR;
    explicit JsonEditor(QWidget *parent = nullptr);
    explicit JsonEditor(QJsonModel *model, QWidget *parent = nullptr);
    virtual ~JsonEditor();
    void setModel(QJsonModel *model);

private slots:
    void checkEnabledEdit();
    void typeChanged();
    QByteArray hashModel() const;
    QByteArray hashText() const;
    bool fileChanged();
    void updateJsonScript();

    void on_btnEdit_clicked();

    void on_actOpen_triggered();

    void on_actSave_triggered();

    void on_actClose_triggered();

    void on_actSynch_triggered();

    void on_actAddChild_triggered();

    void on_actAddSibling_triggered();

private:
    void selectionChanged();
    void timerEvent(QTimerEvent *event) override;

    Ui::JsonEditor *ui;
    QJsonModel *_model = nullptr;
    QString _fileName;
    QByteArray _hashModel;
    int _timerUpdateTable = 0;
};
