#include <QApplication>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMessageBox>
#include <QLabel>
#include <QFile>
#include <QFileSystemWatcher>

void clearLayout(QVBoxLayout *layout) {
    for (int i = layout->count() - 1; i >= 0; --i) {
        QLayoutItem *item = layout->itemAt(i);

        layout->removeItem(item);

        delete item->widget();
        delete item;
    }
}

class TableWindow : public QMainWindow {
Q_OBJECT

private:
    QList<QString> codes;
    QVBoxLayout *pending;
    QVBoxLayout *processing;
    QFileSystemWatcher fileWatcher;
    QFile *file;

    void handleFileChanged() {
        if (file->open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(file);
            codes.clear();
            while(!in.atEnd()) {
                codes.append(in.readLine());
            }
            file->close();
            redrawQueue();
        } else {
            qCritical("Не удалось открыть файл");
        }
    }
protected:
    void closeEvent(QCloseEvent *event) override {
        qApp->quit();
    }

    void redrawQueue() {
        clearLayout(pending);
        clearLayout(processing);
        auto begin = codes.begin();
        auto end = codes.end();

        auto processingText = new QLabel("В обработке:");
        processing->addWidget(processingText);

        if (begin != end) {
            auto processedCodeText = new QLabel(*begin++);
            processing->addWidget(processedCodeText);
        }

        auto pendingText = new QLabel("В очереди:");
        pending->addWidget(pendingText);
        while (begin != end) {
            auto codeText = new QLabel(*begin++);
            pending->addWidget(codeText);
        }
    }

public:
    TableWindow(QWidget *parent, const QString& filePath) : QMainWindow(parent) {
        this->file = new QFile(filePath);
        if(!file->exists()) {
            file->open(QIODevice::WriteOnly);
            file->close();
        }

        auto layout = new QHBoxLayout(nullptr);
        pending = new QVBoxLayout();
        processing = new QVBoxLayout();
        pending->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        processing->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        layout->addLayout(pending);
        layout->addLayout(processing);

        auto centralWidget = new QWidget(this);
        centralWidget->setLayout(layout);

        setCentralWidget(centralWidget);

        fileWatcher.addPath(filePath);
        connect(&fileWatcher, &QFileSystemWatcher::fileChanged, this, &TableWindow::handleFileChanged);

        handleFileChanged();

        this->setWindowTitle("Электронная очередь");
    }

    ~TableWindow() override = default;
};

int main(int argc, char *argv[]) {

    QString path("/tmp/sync");

    QApplication app(argc, argv);
    TableWindow tableWindow(nullptr, path);
    tableWindow.move(200, 200);
    tableWindow.show();

    return QApplication::exec();
}

#include "table.moc"
