/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "debfile.h"

#include <apt-pkg/debfile.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/tagfile.h>

#include <QDebug>

namespace QApt {

class DebFilePrivate
{
    public:
        DebFilePrivate(const QString &path)
            : filePath(path)
            , extractor(0)
        {
            init();
        }

        ~DebFilePrivate()
        {
            delete extractor;
        }

        bool isValid;
        QString filePath;
        debDebFile::MemControlExtract *extractor;
        pkgTagSection controlData;

        void init();
};

void DebFilePrivate::init()
{
    FileFd in(filePath.toStdString(), FileFd::ReadOnly);
    debDebFile deb(in);

    // Extract control data
    extractor = new debDebFile::MemControlExtract("control");
    if(!extractor->Read(deb)) {
      isValid = false;
      return;
    }

    controlData = extractor->Section;
}

DebFile::DebFile(const QString &filePath)
    : d(new DebFilePrivate(filePath))
{
}

DebFile::~DebFile()
{
    delete d;
}

bool DebFile::isValid() const
{
    return d->isValid;
}

QString DebFile::filePath() const
{
    return d->filePath;
}

QString DebFile::packageName() const
{
    return QString::fromStdString(d->controlData.FindS("Package"));
}

QLatin1String DebFile::sourcePackage() const
{
    return QLatin1String(d->controlData.FindS("Source").c_str());
}

QLatin1String DebFile::version() const
{
    return QLatin1String(d->controlData.FindS("Version").c_str());
}

QLatin1String DebFile::architecture() const
{
    return QLatin1String(d->controlData.FindS("Architecture").c_str());
}

QString DebFile::maintainer() const
{
    return QString::fromStdString(d->controlData.FindS("Maintainer"));
}

QLatin1String DebFile::section() const
{
    return QLatin1String(d->controlData.FindS("Section").c_str());
}

QLatin1String DebFile::priority() const
{
    return QLatin1String(d->controlData.FindS("Priority").c_str());
}

QString DebFile::homepage() const
{
    return QString::fromStdString(d->controlData.FindS("Homepage"));
}

QString DebFile::longDescription() const
{
    return QLatin1String(d->controlData.FindS("Description").c_str());
}

QString DebFile::shortDescription() const
{
    QString longDesc = longDescription();

    return longDesc.left(longDesc.indexOf(QLatin1Char('\n')));
}

QString DebFile::controlField(const QLatin1String &field) const
{
    return QString::fromStdString(d->controlData.FindS(field.latin1()));
}

QString DebFile::controlField(const QString &field) const
{
    return controlField(QLatin1String(field.toStdString().c_str()));
}

qint64 DebFile::installedSize() const
{
    QString sizeString = QLatin1String(d->controlData.FindS("Installed-Size").c_str());

    return sizeString.toLongLong();
}

}
