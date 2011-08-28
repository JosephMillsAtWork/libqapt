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

#include "changelog.h"

// Qt includes
#include <QtCore/QRegExp>
#include <QtCore/QSharedData>
#include <QtCore/QStringBuilder>
#include <QtCore/QStringList>
#include <QDebug>

// QApt includes
#include "package.h"

// apt-pkg includes
#include <apt-pkg/strutl.h>

namespace QApt {

class ChangelogEntryPrivate : public QSharedData
{
public:
    ChangelogEntryPrivate(const QString &entryData, const QString &sourcePkg)
        : data(entryData)
    {
        parseData(sourcePkg);
    }

    ChangelogEntryPrivate(const ChangelogEntryPrivate &other)
        : QSharedData(other), data(other.data) {}
    ~ChangelogEntryPrivate() {}

    // Data
    QString data;
    QString version;
    QDateTime issueDate;
    QString description;

    void parseData(const QString &sourcePackage);
};

void ChangelogEntryPrivate::parseData(const QString &sourcePackage)
{
    QStringList lines = data.split('\n');
    QRegExp rxInfo(QString("^%1 \\((.*)\\)(.*)$").arg(QRegExp::escape(sourcePackage)));

    QString versionLine = lines.first();
    lines.removeFirst();
    int pos = rxInfo.indexIn(versionLine);

    QStringList list = rxInfo.capturedTexts();
    if (list.count() > 1) {
        version = list.at(1);
    }

    foreach (const QString &line, lines) {
        // Populate description
        if (line.startsWith(QLatin1String("  "))) {
            description.append(line % '\n');
            continue;
        }

        // NBC Dateline, to catch a QDateTime
        // Spacing matters here
        QRegExp rxDate("^ -- (.+) (<.+>)  (.+)$"); // Chris Hansen
        int pos = rxDate.indexIn(line);
        list = rxDate.capturedTexts();

        if (list.count() > 1) {
            // Why don't you have a seat over there...
            time_t issueTime = -1;
            if (RFC1123StrToTime(list.at(3).toUtf8().data(), issueTime)) {
                issueDate = QDateTime::fromTime_t(issueTime);
                break;
            }
        }
    }
}

ChangelogEntry::ChangelogEntry(const QString &entryData, const QString &sourcePkg)
        : d(new ChangelogEntryPrivate(entryData, sourcePkg))
{
}

ChangelogEntry::ChangelogEntry(const ChangelogEntry &other)
        : d(other.d)
{
}

ChangelogEntry::~ChangelogEntry()
{
}

QString ChangelogEntry::entryText() const
{
    return d->data;
}

QString ChangelogEntry::version() const
{
    return d->version;
}

QDateTime ChangelogEntry::issueDateTime() const
{
    return d->issueDate;
}

QString ChangelogEntry::description() const
{
    return d->description;
}


class ChangelogPrivate : public QSharedData
{
public:
    ChangelogPrivate(const QString &cData, const QString &sData)
        : data(cData), sourcePackage(sData) {}
    ChangelogPrivate(const ChangelogPrivate &other)
        : data(other.data), sourcePackage(other.sourcePackage) {}
    ~ChangelogPrivate() {}

    QString data;
    QString sourcePackage;
};

Changelog::Changelog(const QString &data, const QString &sourcePackage)
        : d(new ChangelogPrivate(data, sourcePackage))
{
}

Changelog::Changelog(const Changelog &other)
        : d(other.d)
{
}

Changelog::~Changelog()
{
}

QString Changelog::text() const
{
    return d->data;
}

ChangelogEntryList Changelog::entries() const
{

    ChangelogEntryList entries;
    QStringList entryTexts;

    foreach (const QString &line, d->data.split('\n')) {
        if (line.startsWith(d->sourcePackage)) {
            entryTexts.append(line % '\n');
            continue;
        }

        int curIndex = entryTexts.count() -1;
        QString curEntry = entryTexts.at(curIndex);

        curEntry.append(line % '\n');
        entryTexts[curIndex] = curEntry;
    }

    foreach (const QString &stanza, entryTexts) {
        ChangelogEntry entry(stanza, d->sourcePackage);

        entries << entry;
    }
}

ChangelogEntryList Changelog::newEntriesSince(const QString &version) const
{
    ChangelogEntryList newEntries;

    foreach (const ChangelogEntry &entry, entries()) {

        int res = Package::compareVersion(entry.version(), version);

        // Add entries newer than the given version
        if (res > 0) {
            newEntries << entry;
        }
    }

    return newEntries;
}

}