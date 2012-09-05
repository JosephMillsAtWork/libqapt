/***************************************************************************
 *   Copyright © 2012 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *   Copyright © 2008-2009 Sebastian Heinlein <devel@glatzor.de>           *
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

#ifndef TRANSACTIONQUEUE_H
#define TRANSACTIONQUEUE_H

#include <QtCore/QObject>
#include <QtCore/QQueue>

class Transaction;

class TransactionQueue : public QObject
{
    Q_OBJECT
public:
    TransactionQueue(QObject *parent = nullptr/*, QAptWorker *worker*/);

    QList<Transaction *> transactions() const;
    Transaction *activeTransaction() const;

private:
    QQueue<Transaction *> m_queue;
    QList<Transaction *> m_pending;
    Transaction *m_activeTransaction;

    Transaction *transactionById(const QString &id);
    
signals:
    void queueChanged();

public slots:
    void addPending(Transaction *trans);
    void enqueue(QString tid);
    void remove(QString tid);

private slots:
    void onTransactionFinished(int exitCode);
    void runNextTransaction();
};

#endif // TRANSACTIONQUEUE_H