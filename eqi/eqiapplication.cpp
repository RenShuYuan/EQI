#include <QIcon>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QProgressDialog>
#include <QApplication>

#include "eqiapplication.h"

eqiApplication::eqiApplication(void)
{
}


eqiApplication::~eqiApplication(void)
{
}

QIcon eqiApplication::getThemeIcon( const QString &theName )
{
    QString defaultThemePath = ":/Resources/images/themes/default/" + theName;

	if ( QFile::exists( defaultThemePath ) )
	{
		//could still return an empty icon if it
		//doesnt exist in the default theme either!
		return QIcon( defaultThemePath );
	}
	else
	{
		return QIcon();
	}
}

QString eqiApplication::getFolder( const QString &folder, const QString &name )
{
	QDir dir(folder);

	if (!dir.exists())
	{
		return "";
	}

	QDirIterator dirIterator(folder, 
		QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, 
		QDirIterator::Subdirectories);

	while (dirIterator.hasNext())
	{
		dirIterator.next();
		QFileInfo file_info = dirIterator.fileInfo();
		qDebug() << file_info.fileName() << "::" << file_info.baseName();
		if (file_info.baseName() == name)
		{
			return file_info.filePath();
		}
	}

	return "";
}

QStringList eqiApplication::searchFiles( const QString &path, QStringList &filters )
{
	QStringList list;

	QDir dir(path);
	if (!dir.exists())
	{
		return list;
	}

	QDirIterator dir_iterator(path, filters, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
	while (dir_iterator.hasNext())
	{
		dir_iterator.next();
		QFileInfo file_info = dir_iterator.fileInfo();
		list.append(file_info.filePath());
	}
    return list;
}

void eqiApplication::setStyle(const QString &style)
{
    QFile qss(style);
    qss.open(QFile::ReadOnly);
    qApp->setStyleSheet(qss.readAll());
    qss.close();
}
