/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "qaptbatch.h"

// Qt includes
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusServiceWatcher>

// KDE includes
#include <KDebug>
#include <KIcon>
#include <KLocale>
#include <KMessageBox>
#include <KWindowSystem>

#include "detailswidget.h"
#include "../../src/globals.h"
#include "../../src/package.h"
#include "../../src/workerdbus.h"

QAptBatch::QAptBatch(QString mode, QStringList packages, int winId)
    : KProgressDialog()
    , m_worker(0)
    , m_watcher(0)
    , m_mode(mode)
    , m_packages(packages)
    , m_detailsWidget(0)
{
    m_worker = new OrgKubuntuQaptworkerInterface("org.kubuntu.qaptworker",
                                                  "/", QDBusConnection::systemBus(),
                                                  this);
    connect(m_worker, SIGNAL(errorOccurred(int, const QVariantMap&)),
            this, SLOT(errorOccurred(int, const QVariantMap&)));
    connect(m_worker, SIGNAL(workerStarted()), this, SLOT(workerStarted()));
    connect(m_worker, SIGNAL(workerEvent(int)), this, SLOT(workerEvent(int)));
    connect(m_worker, SIGNAL(workerFinished(bool)), this, SLOT(workerFinished(bool)));

    m_watcher = new QDBusServiceWatcher(this);
    m_watcher->setConnection(QDBusConnection::systemBus());
    m_watcher->setWatchMode(QDBusServiceWatcher::WatchForOwnerChange);
    m_watcher->addWatchedService("org.kubuntu.qaptworker");

    // Delay auto-show to 10 seconds. We can't disable it entirely, and after
    // 10 seconds people may need a reminder, or something to say we haven't died
    // If auth happens before this, we will manually show when progress happens
    setMinimumDuration(10000);
    // Set this in case we auto-show before auth
    setLabelText(i18nc("@label", "Waiting for authorization"));
    progressBar()->setMaximum(0); // Set progress bar to indeterminate/busy

    m_detailsWidget = new DetailsWidget(this);
    setDetailsWidget(m_detailsWidget);

    if (m_mode == "install") {
        commitChanges(QApt::Package::ToInstall);
    } else if (m_mode == "uninstall") {
        commitChanges(QApt::Package::ToRemove);
    } else if (m_mode == "update") {
        m_worker->updateCache();
    }

    if (winId != 0) {
        KWindowSystem::setMainWindow(this, winId);
    }

    setAutoClose(false);
}

QAptBatch::~QAptBatch()
{
}

void QAptBatch::commitChanges(int mode)
{
    QMap<QString, QVariant> instructionList;

    foreach (const QString &package, m_packages) {
        instructionList.insert(package, mode);
    }

    m_worker->commitChanges(instructionList);
}

void QAptBatch::workerStarted()
{
    // Reset the progresbar's maximum to default
    progressBar()->setMaximum(100);
    connect(m_watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            this, SLOT(serviceOwnerChanged(QString, QString, QString)));

    connect(m_worker, SIGNAL(questionOccurred(int, const QVariantMap&)),
            this, SLOT(questionOccurred(int, const QVariantMap&)));
    connect(m_worker, SIGNAL(downloadProgress(int, int, int)),
            this, SLOT(updateDownloadProgress(int, int, int)));
    connect(m_worker, SIGNAL(commitProgress(const QString&, int)),
            this, SLOT(updateCommitProgress(const QString&, int)));
}
void QAptBatch::errorOccurred(int code, const QVariantMap &args)
{
    QString text;
    QString title;
    QString failedItem;
    QString errorText;
    QString drive;

    switch(code) {
        case QApt::InitError:
            text = i18nc("@label",
                         "The package system could not be initialized, your "
                         "configuration may be broken.");
            title = i18nc("@title:window", "Initialization error");
            raiseErrorMessage(text, title);
            break;
        case QApt::LockError:
            text = i18nc("@label",
                         "Another application seems to be using the package "
                         "system at this time. You must close all other package "
                         "managers before you will be able to install or remove "
                         "any packages.");
            title = i18nc("@title:window", "Unable to obtain package system lock");
            raiseErrorMessage(text, title);
            break;
        case QApt::DiskSpaceError:
            drive = args["DirectoryString"].toString();
            text = i18nc("@label",
                         "You do not have enough disk space in the directory "
                         "at %1 to continue with this operation.", drive);
            title = i18nc("@title:window", "Low disk space");
            raiseErrorMessage(text, title);
            break;
        case QApt::FetchError:
            failedItem = args["FailedItem"].toString();
            errorText = args["ErrorText"].toString();
            text = i18nc("@label",
                         "Failed to download %1\n"
                         "%2", failedItem, errorText);
            title = i18nc("@title:window", "Download failed");
            raiseErrorMessage(text, title);
            break;
        case QApt::CommitError:
            failedItem = args["FailedItem"].toString();
            errorText = args["ErrorText"].toString();
            text = i18nc("@label", "An error occurred while committing changes.");

            if (!failedItem.isEmpty() && !errorText.isEmpty()) {
                text.append("\n\n");
                text.append(i18n("File: %1", failedItem));
                text.append("\n\n");
                text.append(i18n("Error: %1", errorText));
            }

            title = i18nc("@title:window", "Commit error");
            raiseErrorMessage(text, title);
            break;
        case QApt::AuthError:
            text = i18nc("@label",
                         "This operation cannot continue since proper "
                         "authorization was not provided");
            title = i18nc("@title:window", "Authentication error");
            raiseErrorMessage(text, title);
            break;
        case QApt::UntrustedError:
            QStringList untrustedItems = args["UntrustedItems"].toStringList();
            if (untrustedItems.size() == 1) {
                text = i18nc("@label",
                             "The %1 package has not been verified by its author. "
                             "Downloading untrusted packages has been disallowed "
                             "by your current configuration.",
                             untrustedItems.first());
            } else if (untrustedItems.size() > 1) {
                QString failedItemString;
                foreach (const QString &string, untrustedItems) {
                    failedItemString.append("- " + string + '\n');
                }
                text = i18nc("@label",
                             "The following packages have not been verified by "
                             "their authors:"
                             "\n%1\n"
                             "Downloading untrusted packages has "
                             "been disallowed by your current configuration.",
                             failedItemString);
            }
            title = i18nc("@title:window", "Untrusted Packages");
            raiseErrorMessage(text, title);
            break;
    }
}

void QAptBatch::questionOccurred(int code, const QVariantMap &args)
{
    // show() so that closing our question dialog doesn't quit the program
    show();
    QVariantMap response;

    if (code == QApt::InstallUntrusted) {
        QStringList untrustedItems = args["UntrustedItems"].toStringList();

        QString title = i18nc("@title:window", "Untrusted Packages");
        QString text= i18ncp("@label",
                     "The following package has not been verified by its "
                     "author. Installing unverified package represents a "
                     "security risk, as unverified packages can be a "
                     "sign of tampering. Do you wish to continue?",
                     "The following packages have not been verified by "
                     "their authors. Installing unverified packages "
                     "represents a security risk, as unverified packages "
                     "can be a sign of tampering. Do you wish to continue?",
                     untrustedItems.size());
        int result = KMessageBox::No;
        bool installUntrusted = false;

        result = KMessageBox::warningYesNoList(0, text,
                                               untrustedItems, title);
        switch (result) {
            case KMessageBox::Yes:
                installUntrusted = true;
                break;
            case KMessageBox::No:
                installUntrusted = false;
                break;
        }

        response["InstallUntrusted"] = installUntrusted;
        m_worker->answerWorkerQuestion(response);

        if(!installUntrusted) {
            close();
        }
    }
}

void QAptBatch::raiseErrorMessage(const QString &text, const QString &title)
{
    KMessageBox::sorry(0, text, title);
    workerFinished(false);
    close();
}

void QAptBatch::workerEvent(int code)
{
    switch (code) {
        case QApt::CacheUpdateStarted:
            connect(this, SIGNAL(cancelClicked()), m_worker, SLOT(cancelDownload()));
            setWindowTitle(i18nc("@title:window", "Refreshing Package Information"));
            setLabelText(i18nc("@info:status", "Checking for new, removed or upgradeable packages"));
            setButtons(KDialog::Cancel | KDialog::Details);
            show();
            break;
        case QApt::CacheUpdateFinished:
            setLabelText(i18nc("@title:window", "Package information successfully refreshed"));
            disconnect(this, SIGNAL(cancelClicked()), m_worker, SLOT(cancelDownload()));
            progressBar()->setValue(100);
            m_detailsWidget->hide();
            setButtons(KDialog::Close);
            break;
        case QApt::PackageDownloadStarted:
            connect(this, SIGNAL(cancelClicked()), m_worker, SLOT(cancelDownload()));
            setWindowTitle(i18nc("@title:window", "Downloading"));
            setLabelText(i18ncp("@info:status",
                                "Downloading package file",
                                "Downloading package files",
                                m_packages.count()));
            setButtons(KDialog::Cancel | KDialog::Details);
            show();
            break;
        case QApt::PackageDownloadFinished:
            setAllowCancel(false);
            disconnect(this, SIGNAL(cancelClicked()), m_worker, SLOT(cancelDownload()));
            break;
        case QApt::CommitChangesStarted:
            setWindowTitle(i18nc("@title:window", "Installing Packages"));
            m_detailsWidget->hide();
            setButtons(KDialog::Cancel);
            setAllowCancel(false); //Committing changes is uninterruptable (safely, that is)
            show(); // In case no download was necessary
            break;
        case QApt::CommitChangesFinished:
            if (m_mode == "install") {
                setWindowTitle(i18nc("@title:window", "Installation Complete"));
                setLabelText(i18ncp("@label",
                                    "Package successfully installed",
                                    "Packages successfully installed", m_packages.size()));
            } else if (m_mode == "uninstall") {
                setWindowTitle(i18nc("@title:window", "Removal Complete"));
                setLabelText(i18ncp("@label",
                                    "Package successfully uninstalled",
                                    "Packages successfully uninstalled", m_packages.size()));
            }
            progressBar()->setValue(100);
            break;
    }
}

void QAptBatch::workerFinished(bool success)
{
    Q_UNUSED(success);
    disconnect(m_watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
               this, SLOT(serviceOwnerChanged(QString, QString, QString)));

    disconnect(m_worker, SIGNAL(downloadProgress(int, int, int)),
               this, SLOT(updateDownloadProgress(int, int, int)));
    disconnect(m_worker, SIGNAL(commitProgress(const QString&, int)),
               this, SLOT(updateCommitProgress(const QString&, int)));
}

void QAptBatch::serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(name);
    if (oldOwner.isEmpty()) {
        return; // Don't care, just appearing
    }

    if (newOwner.isEmpty()) {
        // Normally we'd handle this in errorOccurred, but if the worker dies it
        // can't really tell us, can it?
        QString text = i18nc("@label", "It appears that the QApt worker has either crashed "
                            "or disappeared. Please report a bug to the QApt maintainers");
        QString title = i18nc("@title:window", "Unexpected Error");

        raiseErrorMessage(text, title);
    }
}

void QAptBatch::updateDownloadProgress(int percentage, int speed, int ETA)
{
    QString timeRemaining;
    int ETAMilliseconds = ETA * 1000;

    // Greater than zero and less than 2 weeks
    if (ETAMilliseconds > 0 && ETAMilliseconds < 14*24*60*60) {
        timeRemaining = KGlobal::locale()->prettyFormatDuration(ETAMilliseconds);
    } else {
        timeRemaining = i18nc("@info:progress Remaining time", "Unknown");
    }

    QString downloadSpeed;
    if (speed == -1) {
        downloadSpeed = i18nc("@info:progress Download rate", "Unknown");
    } else {
        downloadSpeed = i18nc("@info:progress Download rate",
                              "%1/s", KGlobal::locale()->formatByteSize(speed));
    }

    progressBar()->setValue(percentage);
    m_detailsWidget->setTimeText(timeRemaining);
    m_detailsWidget->setSpeedText(downloadSpeed);
}

void QAptBatch::updateCommitProgress(const QString& message, int percentage)
{
    progressBar()->setValue(percentage);
    setLabelText(message);
}

#include "qaptbatch.moc"
