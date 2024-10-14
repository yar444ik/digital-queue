#include <QApplication>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMessageBox>
#include <QLabel>
#include <QFileSystemWatcher>
#include <QFile>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDateTime>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QDebug>
#include <QTimer>

class WorkWindow : public QMainWindow {
Q_OBJECT

private:
    QLabel *statsLabel;
    QLabel *codeLabel;
    QFileSystemWatcher fileWatcher;
    QFile *syncFile;
    QFile *journalFile;

    int processed;

    QSqlDatabase db;
    QSqlQuery *query;
    int row;

protected:
    void closeEvent(QCloseEvent *event) override {
        qApp->quit();
    }

public:
    WorkWindow(QWidget *parent, const QString &syncFilePath, const QString &journalFilePath) : QMainWindow(parent) {
        this->syncFile = new QFile(syncFilePath);
        this->journalFile = new QFile(journalFilePath);

        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("./testDB.db");
        if (db.open()) {
            qDebug("open");
        } else {
            qDebug("close");
        }
        processed = 0;

        query = new QSqlQuery(db);
        query->exec("CREATE TABLE ticket(id INTEGER PRIMARY KEY AUTOINCREMENT, ticket INTEGER, surname TEXT, phone TEXT);");



        if (!syncFile->exists()) {
            syncFile->open(QIODevice::WriteOnly);
            syncFile->close();
        }

        auto layout = new QVBoxLayout(nullptr);
        auto button = new QPushButton("Закончить приём", nullptr);
        auto ticketHeader = new QLabel("Следующий пациент:", nullptr);
        statsLabel = new QLabel("", nullptr);

        codeLabel = new QLabel("Ожидание..", nullptr);
        layout->addWidget(statsLabel);
        layout->addWidget(button);
        layout->addWidget(ticketHeader);
        layout->addWidget(codeLabel);
        layout->setAlignment(Qt::AlignTop);

        auto centralWidget = new QWidget(this);
        centralWidget->setLayout(layout);

        setCentralWidget(centralWidget);

        connect(button, &QPushButton::released, this, &WorkWindow::handleButtonClick);

        fileWatcher.addPath(syncFilePath);
        connect(&fileWatcher, &QFileSystemWatcher::fileChanged, this, &WorkWindow::handleFileChanged);

        handleFileChanged();

        this->setWindowTitle("Приём пациентов");

        auto menuBar = new QMenuBar(nullptr);
        auto helpMenu = new QMenu("Помощь");
        auto helpAction = new QAction("Руководство");
        auto aboutAction = new QAction("О программе");
        helpMenu->addAction(helpAction);
        helpMenu->addAction(aboutAction);
        menuBar->addMenu(helpMenu);
        setMenuBar(menuBar);

        connect(helpAction, &QAction::triggered, this, &WorkWindow::showHelpDialog);
        connect(aboutAction, &QAction::triggered, this, &WorkWindow::showAboutDialog);


        QTimer *timer = new QTimer(this);

        // Connect the timer's timeout signal to a slot
        connect(timer, &QTimer::timeout, this, &WorkWindow::handleFileChanged);

        // Start the timer with a 1000 ms (1 second) interval
        timer->start(1000);

    }

    ~WorkWindow() override = default;

private slots:

    void handleButtonClick() {
        if(!query->exec("DELETE FROM Ticket LIMIT 1")) {
            QMessageBox::warning(this, "Ошибка базы данных", "Не удалось удалить строку");
            return;
        }

        if (!syncFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Не удалось открыть файл", "Не удалось открыть файл");
            return;
        }

        QTextStream in(syncFile);
        QString ticket = in.readLine();
        QString remaining = in.readAll();

        syncFile->close();

        if (!syncFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Не удалось открыть файл", "Не удалось открыть файл");
            return;
        }

        QTextStream out(syncFile);

        out << remaining;

        syncFile->close();
        handleFileChanged();

        QDateTime currentTime = QDateTime::currentDateTime();

        if (!ticket.isNull() && !ticket.isEmpty()) {
            if(journalFile->open(QIODevice::Append | QIODevice::Text)) {
                out.setDevice(journalFile);
                out << "Завершён приём талона " << ticket << " в " << currentTime.toString() << Qt::endl;
                journalFile->close();
            } else {
               qWarning("Не удалось открыть журнал");
            }
            processed += 1;
        }
    };

    void handleFileChanged() {
        query->exec("SELECT ticket, phone, surname FROM Ticket");

        if (query->next()) {
            int id = query->value(0).toInt();
            auto phone = query->value(1).toString();
            auto surname = query->value(2).toString();
            QString ticket = QString("П%1 %2 (Телефон - %3)").arg(id).arg(surname).arg(phone);
            codeLabel->setText(ticket);
        } else {
            codeLabel->setText("Ожидание..");
        }

        statsLabel->setText(QString("Обработано: %1").arg(processed));
    }

    void showHelpDialog() {
        QMessageBox::information(this, "Помощь",
                                 "Нажмите кнопку \"Закончить приём\", чтобы закончить приём и вызвать следующего пациента. Его талон отобразится на дисплее.\n"
                                 "Если пациентов больше нет, то программа будет ждать первого пациента. Его код отобразится на дисплее сразу после этого.");
    }

    void showAboutDialog() {
        QMessageBox::about(this, "О программе", "Очередь пациентов \"СуперВрач\".\n"
                                                "Разработчик: *\n"
                                                "Группа: *\n"
                                                "Все права защищены");
    }
};

int main(int argc, char *argv[]) {

    QString sync("/tmp/sync");
    QString journal("/tmp/journal");

    QApplication app(argc, argv);
    WorkWindow workWindow(nullptr, sync, journal);
    workWindow.move(300, 300);
    workWindow.show();

    return QApplication::exec();
}

#include "work.moc"
