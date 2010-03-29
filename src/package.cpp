/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *   Heavily inspired by Synaptic library code ;-)                         *
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

#include "package.h"

#include <QtCore/QFile>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>

namespace QApt {

class PackagePrivate
{
    public:
        int state;
};

Package::Package(QObject* parent, pkgDepCache *depCache,
                 pkgRecords *records, pkgCache::PkgIterator &packageIter)
        : QObject(parent)
        , d(new PackagePrivate())
        , m_depCache(depCache)
        , m_records(records)
{
    m_packageIter = new pkgCache::PkgIterator(packageIter);
    d->state = state();
}

Package::~Package()
{
}

QString Package::name() const
{
    QString name = QString(m_packageIter->Name());

    if (!name.isNull()) {
        return name;
    } else {
        return QString();
    }
}

QString Package::section() const
{
    QString section = QString(m_packageIter->Section());

    if (!section.isNull()) {
        return section;
    } else {
        return QString();
    }
}

QString Package::sourcePackage() const
{
    QString sourcePackage;
    pkgCache::VerIterator ver = (*m_depCache)[*m_packageIter].CandidateVerIter(*m_depCache);
    pkgRecords::Parser &rec=m_records->Lookup(ver.FileList());
    sourcePackage = QString::fromStdString(rec.SourcePkg());

    return sourcePackage;
}

QString Package::shortDescription() const
{
    QString shortDescription;
    pkgCache::VerIterator ver = (*m_depCache)[*m_packageIter].CandidateVerIter(*m_depCache);
    if (!ver.end()) {
        pkgCache::DescIterator Desc = ver.TranslatedDescription();
        pkgRecords::Parser & parser = m_records->Lookup(Desc.FileList());
        shortDescription = QString::fromStdString(parser.ShortDesc());
        return shortDescription;
    }

    return shortDescription;
}

QString Package::maintainer() const
{
    QString maintainer;
    pkgCache::VerIterator ver = (*m_depCache)[*m_packageIter].CandidateVerIter(*m_depCache);
    if (!ver.end()) {
        pkgRecords::Parser & parser = m_records->Lookup(ver.FileList());
        maintainer = QString::fromStdString(parser.Maintainer());
        return maintainer;
    }
    return maintainer;
}

QString Package::version() const
{
    if ((*m_packageIter)->CurrentVer == 0) {
        pkgDepCache::StateCache & State = (*m_depCache)[*m_packageIter];
        if (State.CandidateVer == 0) {
            return QString();
        } else {
            return QString::fromStdString(State.CandidateVerIter(*m_depCache).VerStr());
        }
    } else {
        return QString::fromStdString(m_packageIter->CurrentVer().VerStr());
    }
}

QString Package::installedVersion() const
{
    QString installedVersion;
    if ((*m_packageIter)->CurrentVer == 0) {
        return QString();
    }
    installedVersion = QString::fromStdString(m_packageIter->CurrentVer().VerStr());
    return installedVersion;
}

QString Package::availableVersion() const
{
    QString availableVersion;
    pkgDepCache::StateCache & State = (*m_depCache)[*m_packageIter];
    if (State.CandidateVer == 0) {
        return QString();
    }

    availableVersion = QString::fromStdString(State.CandidateVerIter(*m_depCache).VerStr());
    return availableVersion;
}

QString Package::priority() const
{
    QString priority;
    pkgCache::VerIterator ver = (*m_depCache)[*m_packageIter].CandidateVerIter(*m_depCache);
    if (ver != 0) {
        priority = QString::fromStdString(ver.PriorityType());
        return priority;
    } else {
        return QString();
    }
}

QStringList Package::installedFilesList() const
{
    QStringList installedFilesList;
    QFile infoFile("/var/lib/dpkg/info/" + name() + ".list");

    if (infoFile.open(QFile::ReadOnly)) {
        QTextStream stream(&infoFile);
        QString line;

        do {
            line = stream.readLine();
            installedFilesList << line;
        } while (!line.isNull());

        // The first item won't be a file
        installedFilesList.removeFirst();
    }

    return installedFilesList;
}

QString Package::longDescription() const
{
    QString longDescription;
    pkgCache::VerIterator ver = (*m_depCache)[*m_packageIter].CandidateVerIter(*m_depCache);

    if (!ver.end()) {
        pkgCache::DescIterator Desc = ver.TranslatedDescription();
        pkgRecords::Parser & parser = m_records->Lookup(Desc.FileList());
        // TODO: Probably needs parsing somewhat, to get rid of non-human-
        // readable lines
        longDescription = QString::fromStdString(parser.LongDesc());
        return longDescription;
    } else {
        return QString();
    }
}

qint32 Package::installedSize() const
{
    pkgCache::VerIterator ver = m_packageIter->CurrentVer();

    if (!ver.end())
        return ver->InstalledSize;
    else
        return -1;
}

qint32 Package::availableInstalledSize() const
{
    pkgDepCache::StateCache & State = (*m_depCache)[*m_packageIter];
    if (State.CandidateVer == 0) {
        return -1;
    }
    return State.CandidateVerIter(*m_depCache)->InstalledSize;
}

qint32 Package::availablePackageSize() const
{
    pkgDepCache::StateCache & State = (*m_depCache)[*m_packageIter];
    if (State.CandidateVer == 0) {
        return -1;
    }

    return State.CandidateVerIter(*m_depCache)->Size;
}

int Package::state()
{
    int packageState = 0;

    pkgDepCache::StateCache &stateCache = (*m_depCache)[*m_packageIter];
    pkgCache::VerIterator ver = m_packageIter->CurrentVer();

    if (stateCache.Install()) {
        packageState |= ToInstall;
    }

    if (stateCache.iFlags & pkgDepCache::ReInstall) {
        packageState |= ToReInstall;
    } else if (stateCache.NewInstall()) { // Order matters here.
        packageState |= NewInstall;
    } else if (stateCache.Upgrade()) {
        packageState |= ToUpgrade;
    } else if (stateCache.Downgrade()) {
        packageState |= ToDowngrade;
    } else if (stateCache.Delete()) {
        packageState |= ToRemove;
        if (stateCache.iFlags & pkgDepCache::Purge) {
            packageState |= ToPurge;
        }
    } else if (stateCache.Keep()) {
        packageState |= ToKeep;
    }

    if (!ver.end()) {
        packageState |= Installed;

        if (stateCache.Upgradable() && stateCache.CandidateVer != NULL) {
            packageState |= Outdated;
            if (stateCache.Keep()) {
                packageState |= Held;
            }
      }

        if (stateCache.Downgrade()) {
            packageState |= ToDowngrade;
        }
    }

    if (stateCache.NowBroken()) {
        packageState |= NowBroken;
    }

    if (stateCache.InstBroken()) {
        packageState |= InstallBroken;
    }

    if ((*m_packageIter)->Flags & (pkgCache::Flag::Important |
                              pkgCache::Flag::Essential)) {
        packageState |= IsImportant;
    }

    if ((*m_packageIter)->CurrentState == pkgCache::State::ConfigFiles) {
        packageState |= ResidualConfig;
    }

    if (stateCache.CandidateVer == 0 ||
        !stateCache.CandidateVerIter(*m_depCache).Downloadable()) {
        packageState |= NotInstallable;
    }

    if (stateCache.Flags & pkgCache::Flag::Auto) {
        packageState |= IsAuto;
    }

    if (stateCache.Garbage) {
        packageState |= IsGarbage;
    }

    if (stateCache.NowPolicyBroken()) {
        packageState |= NowPolicyBroken;
    }

    if (stateCache.InstPolicyBroken()) {
        packageState |= InstallPolicyBroken;
    }

   return packageState /*| _boolFlags*/;
}

bool Package::isInstalled()
{
    pkgCache::VerIterator ver = m_packageIter->CurrentVer();

    if (!ver.end()) {
        return true;
    } else {
        return false;
    }
}

QStringList Package::requiredByList()
{
    QStringList reverseDependsList;

    for(pkgCache::DepIterator it = m_packageIter->RevDependsList(); it.end() != true; it++) {
        reverseDependsList << QString::fromStdString(it.ParentPkg().Name());
    }

    return reverseDependsList;
}

}
