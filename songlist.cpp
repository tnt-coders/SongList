#include "songlist.h"
#include "./ui_songlist.h"

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScrollBar>
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

        // Setup song list table headers
        m_ui->songListTable->setColumnCount(3);
        m_ui->songListTable->setHorizontalHeaderLabels({"Project", "Artist", "Song"});

        auto header = m_ui->songListTable->horizontalHeader();
        header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(2, QHeaderView::Stretch);

        // Hide the "Project" column
        m_ui->songListTable->setColumnHidden(0, true);

        // Initialize data
        this->InitializeLocation();

        // Setup slot connections
        QObject::connect(m_ui->changeButton, &QPushButton::clicked, m_self, &SongList::OnChangeButtonClicked);
        QObject::connect(m_ui->searchBox, &QLineEdit::textChanged, m_self, &SongList::OnSearchBoxTextChanged);
        QObject::connect(m_ui->songListTable, &QTableWidget::cellDoubleClicked, m_self, &SongList::OnSongListTableCellDoubleClicked);
    }

    void OnSearchBoxTextChanged(const QString& text)
    {
        for (int row = 0; row < m_ui->songListTable->rowCount(); ++row)
        {
            bool hidden = true;

            const auto item = m_ui->songListTable->item(row, 0);

            if (item && item->text().contains(text, Qt::CaseInsensitive))
            {
                hidden = false;
            }

            m_ui->songListTable->setRowHidden(row, hidden);
        }
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

    void OnSongListTableCellDoubleClicked(const int row, const int column)
    {
        // Open the REAPER project and if there is an associated guitar pro file open that too
        const auto project = m_ui->songListTable->item(row, 0)->text();
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
        }
    }

    void PopulateSongList()
    {
        // Clear the song list
        m_ui->songListTable->clearContents();

        // Populate the list of available projects
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

        // If projects exist update the table
        if (projects.length() > 0)
        {
            int row = 0;
            for (const auto project : projects)
            {
                // Split artist and song name on "-"
                auto parts = project.split(" - ", Qt::SkipEmptyParts);

                // Trim blank spaces
                for (auto& part : parts)
                {
                    part = part.trimmed();
                }

                // Validate result
                if (parts.size() != 2)
                {
                    qDebug() << "Artist/song name could not be extracted for project: " << project;
                    continue;
                }

                // Extract the artist and song name
                const auto artist = parts[0];
                const auto song = parts[1];

                // Insert a new row
                m_ui->songListTable->insertRow(row);

                // Build the row
                m_ui->songListTable->setItem(row, 0, new QTableWidgetItem(project));
                m_ui->songListTable->setItem(row, 1, new QTableWidgetItem(artist));
                m_ui->songListTable->setItem(row, 2, new QTableWidgetItem(song));

                row++;
            }

            m_ui->songListTable->setSortingEnabled(true);
            m_ui->songListTable->sortItems(1, Qt::AscendingOrder);
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

void SongList::OnSearchBoxTextChanged(const QString& text) {
    m_impl->OnSearchBoxTextChanged(text);
}

void SongList::OnSongListTableCellDoubleClicked(const int row, const int column) {
    m_impl->OnSongListTableCellDoubleClicked(row, column);
}
