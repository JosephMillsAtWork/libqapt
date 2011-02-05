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

#ifndef QAPT_DEBFILE_H
#define QAPT_DEBFILE_H

#include <QtCore/QStringList>

namespace QApt {

/**
 * DebFilePrivate is a class containing all private members of the DebFile class
 */
class DebFilePrivate;

/**
 * The DebFile class is an interface to obtain data from a Debian package
 * archive.
 *
 * @author Jonathan Thomas
 * @since 1.2
 */
class Q_DECL_EXPORT DebFile
{

public:
   /**
    * Default constructor
    */
    DebFile(const QString &filePath);

   /**
    * Default destructor
    */
    ~DebFile();

    bool isValid() const;

   /**
    * Returns the file path of the archive
    *
    * @return The file path of the archive
    */
    QString filePath() const;

   /**
    * Returns the name of the package in this archive
    *
    * @return The name of the package in this archive
    */
    QLatin1String packageName() const;

   /**
    * Returns the source package corresponding to the package in this archive
    *
    * @return The source package
    */
    QLatin1String sourcePackage() const;

   /**
    * Returns the version of the package that this archive provides
    *
    * @return The version of the package this DebFile contains
    */
    QLatin1String version() const;

   /**
    * Returns the CPU architecture that this archive can be installed on
    *
    * For santiy checks, the "APT::Architecture" APT configuration entry
    * can be used to compare to the output of this function. DebFiles with an
    * architecture of "all" can be installed on any architecture
    *
    * @return The archictecure the DebFile is meant for
    */
    QLatin1String architecture() const;

   /**
    * Returns of the maintainer of the package in this archive
    *
    * @return The name and email of the maintainer
    */
    QString maintainer() const;

   /**
    * Returns the categorical section where the archive's package resides
    *
    * @return The section of the archive's package
    */
    QLatin1String section() const;

   /**
    * Returns the update priority of the archive's package
    *
    * @return The update priority
    */
    QLatin1String priority() const;

   /**
    * Returns the homepage of the archive's package
    *
    * @return The homepage
    */
    QString homepage() const;

   /**
    * Returns the one-line description of the archive's package
    *
    * @return The short description
    */
    QString shortDescription() const;

   /**
    * Returns the full description of the archive's package
    *
    * @return The long description
    */
    QString longDescription() const;

   /**
    * Returns the specified field of the package's debian/control file
    *
    * This function can be used to return data from custom control fields
    * which do not have an official function inside APT to retrieve them.
    *
    * @return The value of the specified control field
    */
    QString controlField(const QLatin1String &name) const;

    /** Overload for QString controlField(const QLatin1String &name) const; **/
    QString controlField(const QString &name) const;

   /**
    * Returns the md5sum of the archive
    *
    * @return This archive's md5sum
    */
    QByteArray md5Sum() const;

   /**
    * Returns a list of files that this archive contains
    *
    * @return The file list as a @c QStringList
    */
    QStringList fileList() const;

   /**
    * Returns a list of potential app icons in this archive
    *
    * @return A @c QStringList of icon paths
    */
    QStringList iconList() const;

   /**
    * Returns the installed size of the package that this archive contains
    *
    * @return The package's installed size as a 64-bit integer
    */
    qint64 installedSize() const;

   /**
    * Extracts the data files of the archive to the given directory.
    *
    * If no target directory is given, the current working directory will be
    * used.
    *
    * @param directory The directory to extract to
    *
    * @return @c true on success, @c false on failure
    */
    bool extractArchive(const QString &directory = QString()) const;

   /**
    * Extracts the given file from the package archive to the given destination
    *
    * @param fileName The file to extract from the archive
    * @param destination The destination to place the extracted file
    *
    * @return @c true on success, @c false on failure
    */
    bool extractFileFromArchive(const QString &fileName, const QString &destination) const;

private:
    DebFilePrivate *const d;
};

}

#endif
