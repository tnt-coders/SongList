#pragma once

#include <QMainWindow>
#include <memory>

class SongList : public QMainWindow
{
    Q_OBJECT

public:
    SongList(QWidget *parent = nullptr);
    ~SongList();

public slots:
    void OnChangeButtonClicked();
    void OnSearchBoxTextChanged(const QString& text);
    void OnSongListTableCellDoubleClicked(const int row, const int column);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
