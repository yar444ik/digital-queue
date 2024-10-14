#include <QApplication>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMessageBox>
#include <QTextStream>
#include <QLabel>
#include <QFileSystemWatcher>
#include <QFile>
#include <QDateTime>
#include <QFileInfo>


#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QDebug>
#include <QSqlError>
#include <QLineEdit>

class RegisterWindow : public QMainWindow {
Q_OBJECT

private:
    QFileSystemWatcher fileWatcher;
    QFile *syncFile;
    QFile *journalFile;
    int autoincrement;
    QDateTime latest;

    QSqlDatabase db;
    QSqlQuery *query;
    QLineEdit *surname;
    QLineEdit *phone;

protected:
    void closeEvent(QCloseEvent *event) override {
        qApp->quit();
    }

public:
    RegisterWindow(QWidget *parent, const QString& syncFilePath, const QString& journalFilePath) : QMainWindow(parent) {
        this->syncFile = new QFile(syncFilePath);
        this->journalFile = new QFile(journalFilePath);

        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("./testDB.db");
        if (db.open()){
            qDebug("open");
        } else {
            qDebug("close");
        }

        query = new QSqlQuery(db);
        query->exec("CREATE TABLE ticket(id INTEGER PRIMARY KEY AUTOINCREMENT, ticket INTEGER, surname TEXT, phone TEXT);");


        auto layout = new QVBoxLayout(nullptr);
        auto button = new QPushButton("Получить талон", nullptr);
        auto text = new QLabel("Нажмите кнопку, чтобы получить талон", nullptr);


        surname = new QLineEdit(this);
        surname->setPlaceholderText("Фамилия");

        phone = new QLineEdit(this);
        phone->setPlaceholderText("Номер телефона");

        layout->addWidget(surname);
        layout->addWidget(phone);

        layout->addWidget(button);
        layout->addWidget(text);
        layout->setAlignment(Qt::AlignCenter);

        connect(button, &QPushButton::released, this, &RegisterWindow::handleButtonClick);

        auto centralWidget = new QWidget(this);
        centralWidget->setLayout(layout);

        setCentralWidget(centralWidget);

        this->setWindowTitle("Электронные талоны");

        if (syncFile->exists()) {
            if (syncFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(syncFile);
                QString lastLine;
                while (!in.atEnd()) {
                    lastLine = in.readLine();
                }
                syncFile->close();
                if(!lastLine.isNull()) {
                    bool success;
                    int lastCode = lastLine.mid(1).toInt(&success); // clazy:exclude=qstring-ref
                    if (success) {
                        autoincrement = lastCode;
                        QFileInfo info(*syncFile);
                        latest = info.lastModified();
                    } else {
                        QMessageBox::warning(this, "Не удалось восстановить состояние",
                                             "Не удалось разобрать последний талон");
                        autoincrement = 0;
                    }
                } else {
                    autoincrement = 0;
                }
            } else {
                QMessageBox::warning(this, "Не удалось восстановить состояние", "Не удалось открыть файл");
                autoincrement = 0;
            }
        } else {
            autoincrement = 0;
        }
    }

private slots:

    void handleButtonClick() {
        if(phone->text().isEmpty()) return;
        if(surname->text().isEmpty()) return;

        QDateTime currentTime = QDateTime::currentDateTime();

        if(latest.date() != currentTime.date()) {
            autoincrement = 0;
        }

        autoincrement++;
        query->prepare("INSERT INTO Ticket(ticket, phone, surname) VALUES (:ticket, :phone, :surname)");
        query->bindValue(":ticket", autoincrement);
        query->bindValue(":phone", phone->text());
        query->bindValue(":surname", surname->text());
        qDebug() << "Before insert";
        if(!query->exec()) {
            QMessageBox::warning(this, "Ошибка базы данных", "Не удалось создать строку");
            qDebug() << "Error: Unable to open database" << db.lastError().text();
            return;
        }
        qDebug() << "After insert";
        QString ticket = QString("П%1").arg(autoincrement);
        if(syncFile->open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(syncFile);
            out << ticket << Qt::endl;
            syncFile->close();
            latest = currentTime;

            if(journalFile->open(QIODevice::Append | QIODevice::Text)) {
                out.setDevice(journalFile);
                out << "Получен талон " << ticket << " в " << currentTime.toString() << Qt::endl;
                journalFile->close();
            } else {
                qWarning("Не удалось открыть журнал");
            }

            QMessageBox::information(this, "Талон создан", QString("Ваш талон: %1. Дождитесь очереди на табло.").arg(ticket));
        } else {
            QMessageBox::warning(this, "Не удалось создать талон", "Не удалось открыть файл");
        }
    };
};

int main(int argc, char *argv[]) {

    QString sync("/tmp/sync");
    QString journal("/tmp/journal");

    QApplication app(argc, argv);
    RegisterWindow registerWindow(nullptr, sync, journal);
    registerWindow.move(100, 100);
    registerWindow.show();

    return QApplication::exec();
}

#include "register.moc"
