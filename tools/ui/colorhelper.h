#ifndef COLORHELPER_H
#define COLORHELPER_H

#include <QObject>
#include <QColor>

class ColorHelper : public QObject{
    Q_OBJECT
public:
    explicit ColorHelper(QObject *parent = nullptr);
    Q_INVOKABLE QColor colorForPath(const QString &path) const;
};

#endif // COLORHELPER_H
