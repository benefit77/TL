#ifndef LIBPATHHELPER_H
#define LIBPATHHELPER_H

#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QProcessEnvironment>

// 获取搜索路径列表：
// 1. AppImage 所在目录（如果从 AppImage 运行）
// 2. 应用程序所在目录
// 3. 应用程序所在目录的子目录
inline QStringList getLibSearchPaths(const QString &subDir = QString())
{
    QStringList paths;

    // 1. AppImage 所在目录
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains("APPIMAGE")) {
        QString appImagePath = env.value("APPIMAGE");
        QString appImageDir = QFileInfo(appImagePath).absolutePath();
        if (!subDir.isEmpty())
            paths << (appImageDir + "/" + subDir);
        paths << appImageDir;
    }

    // 2. 应用程序所在目录
    QString appDir = QCoreApplication::applicationDirPath();
    if (!subDir.isEmpty())
        paths << (appDir + "/" + subDir);
    paths << appDir;

    // 3. 当前工作目录
    if (!subDir.isEmpty())
        paths << (QDir::currentPath() + "/" + subDir);
    paths << QDir::currentPath();

    return paths;
}

// 在多个路径中查找指定文件，返回第一个找到的完整路径
inline QString findLibFile(const QString &fileName, const QString &subDir = QString())
{
    QStringList paths = getLibSearchPaths(subDir);
    for (const QString &path : paths) {
        QString fullPath = path + "/" + fileName;
        if (QFileInfo::exists(fullPath))
            return QFileInfo(fullPath).absoluteFilePath();
    }
    return QString();
}

#endif // LIBPATHHELPER_H
