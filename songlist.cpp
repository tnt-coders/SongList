#include "songlist.h"
#include "./ui_songlist.h"

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QUrl>

const static auto APP_DATA = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

struct SongList::Impl
{
    Impl(SongList* self)
        : m_self(self)
        , m_ui(std::make_unique<Ui::MainWindow>())
        , m_location("C:/")
    {
        m_ui->setupUi(m_self);

        // Initialize data
        this->InitializeLocation();

        // Setup slot connections
        QObject::connect(m_ui->changeButton, &QPushButton::clicked, m_self, &SongList::OnChangeButtonClicked);
        QObject::connect(m_ui->openButton, &QPushButton::clicked, m_self, &SongList::OnOpenButtonClicked);
    }

    void OnChangeButtonClicked()
    {
        QFileDialog fileDialog(m_self);
        fileDialog.setFileMode(QFileDialog::Directory);

        fileDialog.setDirectory(m_location.absolutePath());

        if (fileDialog.exec())
        {
            const auto newLocation = fileDialog.selectedFiles()[0];
            this->UpdateLocation(newLocation);
        }
    }

    void OnOpenButtonClicked()
    {
        // Open the REAPER project and if there is an associated guitar pro file open that too
        const auto project = m_ui->songListComboBox->currentText();
        QDir projectDir(m_location.absolutePath() + "/" + project);

        const auto files = projectDir.entryList(QDir::Files);
        for (const auto file : files)
        {
            if (file.endsWith(".rpp") || file.endsWith(".gp"))
            {
                const auto absoluteFilePath = projectDir.absolutePath() + "/" + file;
                QDesktopServices::openUrl(QUrl::fromLocalFile(absoluteFilePath));
            }
        }
    }

private:
    void InitializeLocation()
    {
        QFile locationFile(APP_DATA + "/location.dat");
        if (locationFile.open(QIODevice::ReadOnly))
        {
            QTextStream inputStream(&locationFile);
            const auto location = inputStream.readLine();
            locationFile.close();

            this->UpdateLocation(location, true);
        }
        else
        {
            m_ui->locationValueLabel->setText("Select Location...");
        }
    }

    void UpdateLocation(const QDir& location, bool init=false)
    {
        if (location.exists())
        {
            // Save the location to a file to be recalled on next startup
            if (!init)
            {
                QFile locationFile(APP_DATA + "/location.dat");
                if (locationFile.open(QIODevice::WriteOnly))
                {
                    QTextStream outputStream(&locationFile);
                    outputStream << location.absolutePath();
                    locationFile.close();
                }
                else
                {
                    QMessageBox::warning(m_self, "Warning", "Failed to save location to AppData");
                }
            }

            m_location = location;
            m_ui->locationValueLabel->setText(m_location.absolutePath());
            this->PopulateSongList();
        }
        else
        {
            m_ui->locationValueLabel->setText("Select location...");
            m_ui->songListComboBox->setEnabled(false);
        }
    }

    void PopulateSongList()
    {
        // Clear the song list
        m_ui->songListComboBox->clear();

        // Populate the song list
        auto projects = m_location.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        // Remove projects starting with "__"
        projects.removeIf([](const QString& project){
            return project.startsWith("__");
        });

        // Remove projects with no REAPER (.rpp) file
        projects.removeIf([&](const QString& project){
            QDir projectDir(m_location.absolutePath() + "/" + project);

            const auto files = projectDir.entryList(QDir::Files);
            for (const auto file : files)
            {
                if (file.endsWith(".rpp"))
                {
                    return false;
                }
            }

            return true;
        });

        // If projects exist update the combo box and enable the "open" button
        if (projects.length() > 0)
        {
            m_ui->songListComboBox->addItems(projects);
            m_ui->songListComboBox->setEnabled(true);
            m_ui->openButton->setEnabled(true);
        }
        else
        {
            m_ui->songListComboBox->addItem("No projects found.");
            m_ui->songListComboBox->setEnabled(false);
            m_ui->openButton->setEnabled(false);
        }
    }

private:
    SongList* m_self;
    std::unique_ptr<Ui::MainWindow> m_ui;

    QDir m_location;
};

SongList::SongList(QWidget *parent)
    : QMainWindow(parent)
    , m_impl(std::make_unique<Impl>(this))
{}

SongList::~SongList() = default;

void SongList::OnChangeButtonClicked() {
    m_impl->OnChangeButtonClicked();
}

void SongList::OnOpenButtonClicked() {
    m_impl->OnOpenButtonClicked();
}
