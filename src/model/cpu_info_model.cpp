/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "cpu_info_model.h"

#include "system/cpu_set.h"
#include "cpu_list_model.h"
#include "system/device_db.h"
#include "system/system_monitor.h"

CPUInfoModel::CPUInfoModel(const TimePeriod &period, QObject *parent)
    : QObject(parent)
    , m_period(period)
    , m_overallStatSample(new CPUStatSample(period))
    , m_overallUsageSample(new CPUUsageSample(period))
    , m_loadAvgSampleDB(new LoadAvgSample(period))
    , m_cpuSet {}
{
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
    m_sysInfo = monitor->sysInfo();
    m_cpuSet = monitor->deviceDB()->cpuSet();

    connect(monitor, &SystemMonitor::statInfoUpdated, this, &CPUInfoModel::updateModel);
}

QString CPUInfoModel::uptime() const
{
    QString buffer;
    time_t uptime = m_sysInfo->uptime().tv_sec;
    long days = uptime / 86400;
    long hours = (uptime - days * 86400) / 3600;
    long mins = (uptime - days * 86400 - hours * 3600) / 60;
    // TODO: move tr to i18n.cpp
    buffer = QString("%1, %2:%3")
             .arg(QCoreApplication::translate("up %1 days(s)", "SysInfo.Uptime", nullptr, int(days)))
             .arg(hours)
             .arg(mins);
    return buffer;
}

void CPUInfoModel::updateModel()
{
    m_overallStatSample->addSample(new CPUStatSampleFrame(m_sysInfo->uptime(), std::make_shared<struct cpu_stat_t>(*m_cpuSet->stat())));

    m_overallUsageSample->addSample(new CPUUsageSampleFrame(m_sysInfo->uptime(), std::make_shared<struct cpu_usage_t>(*m_cpuSet->usage())));

    m_loadAvgSampleDB->addSample(new LoadAvgSampleFrame(m_sysInfo->uptime(), std::make_shared<struct load_avg_t>(*m_sysInfo->loadAvg())));

    for (auto &cpuname : m_cpuSet->cpuLogicName()) {
        if (m_singleUsageSample.contains(cpuname)) {
            m_singleUsageSample[cpuname]->addSample(new CPUUsageSampleFrame(m_sysInfo->uptime(), std::make_shared<struct cpu_usage_t>(*m_cpuSet->usageDB(cpuname))));
        } else {
            auto smaple = std::make_shared<Sample<cpu_usage_t>>(m_period);
            smaple->addSample(new CPUUsageSampleFrame(m_sysInfo->uptime(), std::make_shared<struct cpu_usage_t>(*m_cpuSet->usageDB(cpuname))));
            m_singleUsageSample.insert(cpuname, smaple);
        }

//        auto model = std::make_shared<CPUStatModel>(m_period, this);
//        if (model && model->m_statSampleDB)
//            model->m_statSampleDB->addSample(new CPUStatSampleFrame(m_sysInfo->uptime(), std::make_shared<struct cpu_stat_t>(*m_cpuSet->statDB(cpuname))));

//        if (model && model->m_usageSampleDB)
//            model->m_usageSampleDB->addSample(new CPUUsageSampleFrame(m_sysInfo->uptime(), std::make_shared<struct cpu_usage_t>(*m_cpuSet->usageDB(cpuname))));

//        m_cpuListModel->m_statModelDB[info.logicalName()] = model;
    } // ::for

    emit modelUpdated();
} // ::updateModel

QString CPUInfoModel::cpuFreq() const
{
    return formatHz(m_cpuSet->curfreq());
}

QString CPUInfoModel::maxFreq() const
{
    return formatHz(m_cpuSet->maxfreq());
}

QString CPUInfoModel::minFreq() const
{
    return formatHz(m_cpuSet->minfreq());
}

QString CPUInfoModel::model() const
{
    return m_cpuSet->cpuModel();
}

QString CPUInfoModel::vendor() const
{
    return m_cpuSet->cpuVendor();
}

uint CPUInfoModel::nCores() const
{
    return m_cpuSet->cores();
}

uint CPUInfoModel::nSockets() const
{
    return m_cpuSet->sockets();
}

uint CPUInfoModel::nProcessors() const
{
    return m_cpuSet->processors();
}

QString CPUInfoModel::virtualization() const
{
    return m_cpuSet->virtualization();
}

QString CPUInfoModel::l1iCache() const
{
    return formatUnit({m_cpuSet->l1iCache()}, B, 0);
}

QString CPUInfoModel::l1dCache() const
{
    return formatUnit({m_cpuSet->l1dCache()}, B, 0);
}

QString CPUInfoModel::l2Cache() const
{
    return formatUnit({m_cpuSet->l2Cache()}, B, 0);
}

QString CPUInfoModel::l3Cache() const
{
    return formatUnit({m_cpuSet->l3Cache()}, B, 0);
}

QList<qreal> CPUInfoModel::cpuPercentList() const
{
    QList<qreal> percentList;
    const auto &usageSampleList = m_singleUsageSample.values();
    for (const auto &sample : usageSampleList) {
        const auto &pair = sample->recentSamplePair();
        percentList << CPUUsageSampleFrame::cpupc(pair.first, pair.second);
    }
    return percentList;
}

qreal CPUInfoModel::cpuAllPercent() const
{
    auto pair = m_overallUsageSample->recentSamplePair();
    return CPUUsageSampleFrame::cpupc(pair.first, pair.second);
}

QString CPUInfoModel::loadavg() const
{
    QString buffer {};
    buffer << *m_sysInfo->loadAvg();
    return buffer;
}

uint CPUInfoModel::nProcesses() const
{
    return m_sysInfo->nprocesses();
}

uint CPUInfoModel::nThreads() const
{
    return m_sysInfo->nthreads();
}

uint CPUInfoModel::nFileDescriptors() const
{
    return m_sysInfo->nfds();
}

QString CPUInfoModel::hostname() const
{
    return m_sysInfo->hostname();
}

QString CPUInfoModel::osType() const
{
    return m_sysInfo->arch();
}

QString CPUInfoModel::osVersion() const
{
    return m_sysInfo->version();
}