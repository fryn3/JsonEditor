#include "jsoneditor.h"
#include "ui_jsoneditor.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QCborValue>

const QMap<QJsonValue::Type, QString> JsonEditor::TYPES_STR {
    { QJsonValue::Null, "Null"},
    { QJsonValue::Bool, "Bool"},
    { QJsonValue::Double, "Double"},
    { QJsonValue::String, "String"},
    { QJsonValue::Array, "Array"},
    { QJsonValue::Object, "Object"},
};

JsonEditor::JsonEditor(QWidget *parent) : JsonEditor(new QJsonModel(), parent) { }

JsonEditor::JsonEditor(QJsonModel *model, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::JsonEditor), _model(nullptr), _hashModel() {
    ui->setupUi(this);
    setModel(model);
    ui->treeView->setModel(model);
    ui->teScript->setPlainText(_model->toByteArray(true));
    connect(ui->teScript, &QPlainTextEdit::textChanged, this, [this] {
        if (ui->actAuto->isChecked()) {
            if (_timerUpdateTable) {
                killTimer(_timerUpdateTable);
            }
            _timerUpdateTable = startTimer(TIME_UPDATE_TABLE);
        }
        qDebug() << hashText() << hashModel()
                 << (hashText() == hashModel());
        ui->actSynch->setEnabled(hashText() != hashModel());
    });
    connect(ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &JsonEditor::selectionChanged);
    connect(ui->leKey, &QLineEdit::textEdited, this, &JsonEditor::checkEnabledEdit);
    connect(ui->cbType, &QComboBox::currentTextChanged, this, &JsonEditor::checkEnabledEdit);
    connect(ui->leValue, &QLineEdit::textEdited, this, &JsonEditor::checkEnabledEdit);
    ui->treeView->selectionModel();
    for (auto it = TYPES_STR.cbegin(); it != TYPES_STR.cend(); ++it) {
        ui->cbType->addItem(it.value(), it.key());
    }
    connect(ui->cbType, &QComboBox::currentTextChanged, this, &JsonEditor::typeChanged);
    connect(ui->btnAddChild, &QPushButton::clicked, this, [this] {
        _model->addChildren(ui->treeView->selectionModel()->currentIndex());
    });
    connect(ui->btnAddSibling, &QPushButton::clicked, this, [this] {
        _model->addSibling(ui->treeView->selectionModel()->currentIndex());
    });
    fileChanged();
}

JsonEditor::~JsonEditor() {
    delete ui;
}

void JsonEditor::setModel(QJsonModel *model) {
    if (_model) {
        disconnect(_model, &QJsonModel::dataChanged, this, &JsonEditor::updateJsonScript);
        disconnect(_model, &QJsonModel::modelReset, this, &JsonEditor::updateJsonScript);
        disconnect(_model, &QAbstractItemModel::dataChanged, this, &JsonEditor::selectionChanged);
        disconnect(_model, &QAbstractItemModel::modelReset, this, &JsonEditor::selectionChanged);
    }
    _model = model;
    connect(_model, &QJsonModel::dataChanged, this, &JsonEditor::updateJsonScript);
    connect(_model, &QJsonModel::modelReset, this, &JsonEditor::updateJsonScript);
    connect(_model, &QAbstractItemModel::dataChanged, this, &JsonEditor::selectionChanged);
    connect(_model, &QAbstractItemModel::modelReset, this, &JsonEditor::selectionChanged);
    _hashModel = hashModel();
    fileChanged();
}

void JsonEditor::checkEnabledEdit() {
    const auto item = static_cast<QJsonTreeItem*>
            (ui->treeView->selectionModel()->currentIndex().internalPointer());
    if (!item) { return; }
    Q_ASSERT(item->parent());
    switch (item->type()) {
    case QJsonValue::Null:
    case QJsonValue::Array:
    case QJsonValue::Object:
        ui->btnEdit->setEnabled(item->key() != ui->leKey->text()
                            || item->type() != ui->cbType->currentData());
        return;
    case QJsonValue::Bool:
    case QJsonValue::Double:
        ui->btnEdit->setEnabled(
                    item->key() != ui->leKey->text()
                    || item->type() != ui->cbType->currentData()
                    || QString("%1").arg(item->value().toDouble())
                                                   != ui->leValue->text());
        return;
    case QJsonValue::String:
        ui->btnEdit->setEnabled(
                    item->key() != ui->leKey->text()
                    || item->type() != ui->cbType->currentData()
                    || item->value().toString() != ui->leValue->text());
        return;
    default:
        Q_ASSERT(false && "bad item type");
    }
}

void JsonEditor::typeChanged() {
    const auto item = static_cast<QJsonTreeItem*>
            (ui->treeView->selectionModel()->currentIndex().internalPointer());
    ui->btnAddChild->setEnabled(item->isArrayOrObject() || item->type() == QJsonValue::Null);
    switch (ui->cbType->currentData().toInt()) {
    case QJsonValue::Null:
        ui->leValue->clear();
        ui->leValue->setEnabled(false);
        break;
    case QJsonValue::Array:
    case QJsonValue::Object:
        ui->leValue->clear();
        ui->leValue->setEnabled(false);
        break;
    case QJsonValue::Bool:
    case QJsonValue::Double:
        ui->leValue->setText(QString("%1").arg(item->value().toDouble()));
        ui->leValue->setEnabled(true);
        break;
    case QJsonValue::String:
        ui->leValue->setText(item->value().toString());
        ui->leValue->setEnabled(true);
        break;
    default:
        Q_ASSERT(false && "bad data cb");
    }
}

QByteArray JsonEditor::hashModel() const {
    return QCryptographicHash::hash(_model->toByteArray(true), QCryptographicHash::Algorithm::Md5);
}

QByteArray JsonEditor::hashText() const {
    return QCryptographicHash::hash(ui->teScript->toPlainText().toUtf8(), QCryptographicHash::Algorithm::Md5);
}

bool JsonEditor::fileChanged() {
    if (hashModel() == _hashModel) {
        setWindowTitle(_fileName.isEmpty() ? objectName() : _fileName);
        return false;
    } else {
        setWindowTitle((_fileName.isEmpty() ? objectName() : _fileName) + "*");
        return true;
    }
}

void JsonEditor::updateJsonScript() {
    ui->teScript->setPlainText(_model->toByteArray(true));
    fileChanged();
}

void JsonEditor::selectionChanged() {
    const auto item = static_cast<QJsonTreeItem*>
            (ui->treeView->selectionModel()->currentIndex().internalPointer());
    if (!item) {
        qDebug() << __PRETTY_FUNCTION__ << "!item";
        return;
    }
    auto parentItem = item->parent();
    if (!parentItem) {
        return;
    }

    ui->leKey->setText(item->key());
    ui->leKey->setEnabled(parentItem->type() != QJsonValue::Array);
    switch (item->type()) {
    case QJsonValue::Double:
    case QJsonValue::Bool:
        ui->btnAddChild->setEnabled(false);
        ui->leValue->setText(QString("%1").arg(item->value().toDouble()));
        ui->leValue->setEnabled(true);
        break;
    case QJsonValue::String:
        ui->btnAddChild->setEnabled(false);
        ui->leValue->setText(item->value().toString());
        ui->leValue->setEnabled(true);
        break;
    case QJsonValue::Array:
    case QJsonValue::Null:
    case QJsonValue::Object:
        ui->btnAddChild->setEnabled(true);
        ui->leValue->clear();
        ui->leValue->setEnabled(false);
        break;
    default:
        Q_ASSERT(false && "bad item type");
    }
    ui->cbType->setCurrentText(TYPES_STR[item->type()]);
    ui->btnEdit->setEnabled(false);
}

void JsonEditor::timerEvent(QTimerEvent *event) {
    if (_timerUpdateTable == event->timerId()) {
        killTimer(_timerUpdateTable);
        _timerUpdateTable = 0;
        on_actSynch_triggered();
    }
}

void JsonEditor::on_btnEdit_clicked() {
    auto currIndex = ui->treeView->selectionModel()->currentIndex();
    auto item = static_cast<QJsonTreeItem*>(currIndex.internalPointer());
    bool success = false;
    if (item->key() != ui->leKey->text()) {
        auto ind0 = currIndex.sibling(currIndex.row(), 0);
        success = _model->setData(ind0, ui->leKey->text());
        if (!success) {
            ui->statusbar->showMessage(QString("Can't edit key: %1 -> %2").arg(item->key()).arg(ui->leKey->text()), 5);
            return;
        }
    }
    QString strValue = ui->leValue->text();
    auto ind1 = currIndex.sibling(currIndex.row(), 1);
    switch (ui->cbType->currentData().toInt()) {
    case QJsonValue::Null:
    case QJsonValue::Object:
        _model->setData(ind1, QVariant());
        break;
    case QJsonValue::Array:
        _model->setData(ind1, QVariant(QStringList()));
        break;
    case QJsonValue::Bool:
        _model->setData(ind1, bool(strValue.toInt()));
        break;
    case QJsonValue::Double:
        _model->setData(ind1, strValue.toDouble());
        break;
    case QJsonValue::String:
        _model->setData(ind1, strValue);
        break;
    default:
        Q_ASSERT(false && "bad ComboBox type");
    }
    selectionChanged();
}

void JsonEditor::on_actOpen_triggered() {
    on_actClose_triggered();
    _fileName = QFileDialog::getOpenFileName(
        this, tr("Open file"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        tr("JSON files (*.json)"));
    if (_fileName.isEmpty()) {
        ui->statusbar->showMessage("File didn't selecte", 5);
        return;
    }
    _model->load(_fileName);
    _hashModel = hashModel();
    fileChanged();
}

void JsonEditor::on_actSave_triggered() {
    if (!fileChanged()) {
        ui->statusbar->showMessage("File didn't change!", 5);
        return;
    }
    if (_fileName.isEmpty()) {
        _fileName = QFileDialog::getSaveFileName(
            this, tr("Save file"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("JSON files (*.json)"));
    }
    if (_fileName.isEmpty()) {
        ui->statusbar->showMessage("Save canceled!", 5);
        return;
    }
    QFile f(_fileName);
    if (!f.open(QIODevice::WriteOnly)) {
        ui->statusbar->showMessage("Can't open file", 5);
        return;
    }
    f.write(_model->toByteArray(true));
    _hashModel = hashModel();
    fileChanged();
}

void JsonEditor::on_actClose_triggered() {
    auto clearFn = [this] {
        _fileName = nullptr;
        _model->clear();
        _hashModel = hashModel();
        fileChanged();
    };
    if (!fileChanged()) {
        clearFn();
        return;
    }
    int ret = QMessageBox::warning(this, objectName(),
                                   "Do you want to save changes?",
                                   QMessageBox::Yes| QMessageBox::No
                                   | QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::No:
        clearFn();
        break;
    case QMessageBox::Cancel:
        return;
    case QMessageBox::Yes:
        on_actSave_triggered();
        clearFn();
        break;
    default:
        qDebug() << __PRETTY_FUNCTION__ << "bad ret"<< ret;
    }
}

void JsonEditor::on_actSynch_triggered() {
    QJsonParseError parse_error;
    auto jdoc = QJsonDocument::fromJson(ui->teScript->toPlainText().toUtf8(), &parse_error);
    if (parse_error.error) {
        ui->statusbar->showMessage(parse_error.errorString());
        return;
    }
    ui->statusbar->showMessage("");
    if (jdoc != _model->toJsonDoc()) {
        _model->load(jdoc);
    }
}
