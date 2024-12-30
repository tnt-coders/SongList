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
    void OnOpenButtonClicked();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
