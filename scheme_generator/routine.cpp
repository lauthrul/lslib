#include "stdafx.h"
#include "routine.h"
#include "lottery/lottery.h"
#include <memory>

//////////////////////////////////////////////////////////////////////////
#define ROUTINE_INTERVAL            5   // second
#define HISTORY_CODE_CACHE_SIZE     100
enum
{
    TID_HISTORY_CODE_UPDATED = TID_BASE + 2000,
};

CRoutine::CRoutine() : CThread(true)
{
}

CRoutine::~CRoutine()
{

}

void CRoutine::Run()
{
    Resume();
    CThread::Run();
}

void CRoutine::OnExecute()
{
    Routine();
}

int CRoutine::HandleMessage(msgid_t uMsg, wparam_t wParam /*= 0*/, lparam_t lParam /*= 0*/)
{
    STask* pTask = (STask*)lParam;
    if (pTask == NULL || pTask->pResp == NULL) return -1;

    switch (uMsg)
    {
        case TID_GET_HISTORY_CODE:
            {
                SHistroyCodeResp* pResp = (SHistroyCodeResp*)pTask->pResp;
                if (pResp != NULL && pResp->nResultCode == 0)
                {
                    string& rfLottery = pResp->strLottery;
                    string& rfCurrentIssue = pResp->strCurIssue;
                    MapHistoryCode& rfHistoryResp = pResp->mapHistoryCode;
                    MapHistoryCode& rfHistory = m_mapAllHistoryCode[pResp->strLottery];

                    bool bNewResult = false;
                    MapHistoryCode newHistory;
                    if (rfHistory.empty())
                        newHistory = rfHistoryResp;
                    else
                    {
                        SHistoryCodeItem codeBegin = rfHistoryResp.begin()->second;
                        MapHistoryCode::iterator it = rfHistory.find(codeBegin.strIssue);
                        if (it == rfHistory.end())
                            newHistory = rfHistoryResp;
                        else
                        {
                            size_t dist = std::distance(it, rfHistory.end());
                            if (dist <= rfHistoryResp.size())
                            {
                                it = rfHistoryResp.begin();
                                std::advance(it, dist);
                                if (it != rfHistoryResp.end())
                                {
                                    for (MapHistoryCode::iterator idx = it; idx != rfHistoryResp.end(); idx++)
                                        newHistory[idx->first] = idx->second;
                                }
                            }
                        }
                    }

                    if (!newHistory.empty())
                    {
                        if (rfHistory.size() > HISTORY_CODE_CACHE_SIZE)
                        {
                            int nIndex = rfHistory.size() - HISTORY_CODE_CACHE_SIZE;
                            while (nIndex > 0)
                            {
                                rfHistory.erase(rfHistory.begin());
                                nIndex--;
                            }
                        }

                        for (MapHistoryCode::iterator it = newHistory.begin(); it != newHistory.end(); it++)
                            rfHistory[it->first] = it->second;

                        SHistroyCode historyCode;
                        historyCode.strLottery = rfLottery;
                        historyCode.strCurIssue = rfCurrentIssue;
                        historyCode.mapHistoryCode = newHistory;
                        PostMessage(TID_HISTORY_CODE_UPDATED, (wparam_t)new string(rfLottery), (lparam_t)new SHistroyCode(historyCode)); // remember to delete
                        INFO_LOG(g_pLogger, "=== lottery[%s] opened [%d] new issues! latest opened issue[%s], current issue[%s] ===",
                                 rfLottery.c_str(), newHistory.size(), newHistory.rbegin()->first.c_str(), rfCurrentIssue.c_str());
                    }
                }
            }
            break;
        case TID_HISTORY_CODE_UPDATED:
            {
                auto_ptr<string> apLottery((string*)wParam);
                auto_ptr<SHistroyCode> apHistoryCode((SHistroyCode*)lParam);
                string& rfCurIssue = apHistoryCode->strCurIssue;
                MapHistoryCode& rfNewHisotrys = apHistoryCode->mapHistoryCode;

                list<SubScheme> lstSubSchemeUpdated;
                map<int, SubScheme> mapSubSchemes;
                g_dbWrapper.GetSubSchemesByLottery(mapSubSchemes, apLottery->c_str());
                for (map<int, SubScheme>::iterator it = mapSubSchemes.begin(); it != mapSubSchemes.end(); it++)
                {
                    SubScheme& rfSubScheme = it->second;
                    map<int, SchemeDetail> mapSchemeDetails;
                    g_dbWrapper.GetSchemeDetailsBySubSchemeID(mapSchemeDetails, rfSubScheme.nID, rfNewHisotrys.begin()->first.c_str());

                    SchemeDetail* pLastSchemeDetail = NULL;
                    auto_ptr<SchemeDetail> apLastSchemeDetail;
                    MapHistoryCode::iterator it_hlast = rfNewHisotrys.begin();
                    if (!mapSchemeDetails.empty())
                    {
                        SchemeDetail lastSchemeDetail = mapSchemeDetails.rbegin()->second; // 找到最后一期记录
                        if (lastSchemeDetail.strIssue >
                                rfNewHisotrys.rbegin()->second.strIssue) // 最后一期记录大于新开奖范围（一般是当期已经产生记录，但还未开奖，然后在本期开奖前重启了软件）
                            continue;

                        it_hlast = rfNewHisotrys.find(lastSchemeDetail.strIssue);
                        if (lastSchemeDetail.strOpenCode.empty() && it_hlast != rfNewHisotrys.end()) // 未开奖，且在当前新奖期范围内
                        {
                            lastSchemeDetail.strOpenCode = it_hlast->second.strCode;
                            lastSchemeDetail.bWin = lottery::CheckWin(lastSchemeDetail);
                            lottery::StatisticSubScheme(rfSubScheme, lastSchemeDetail);

                            g_dbWrapper.UpdateSchemeDetailResult(lastSchemeDetail);

                            pLastSchemeDetail = new SchemeDetail(lastSchemeDetail);
                            apLastSchemeDetail = auto_ptr<SchemeDetail>(pLastSchemeDetail);

                            INFO_LOG(g_pLogger, "=== update scheme detail[merchant_id:%d, scheme_id:%d, sub_scheme_id:%d, lottery:%s, issue:%d, open_code:%s, win:%d] ===",
                                     lastSchemeDetail.nMerchantID, lastSchemeDetail.nSchemeID, lastSchemeDetail.nSubSchemeID,
                                     lastSchemeDetail.strLottery.c_str(), lastSchemeDetail.strIssue.c_str(), lastSchemeDetail.strOpenCode.c_str(), lastSchemeDetail.bWin);

                            it_hlast++;
                        }
                    }

                    // 根据新的历史开奖生成计划详情
                    mapSchemeDetails.clear();
                    int nIndex = 0;
                    for (MapHistoryCode::iterator it_h = it_hlast; it_h != rfNewHisotrys.end(); it_h++)
                    {
                        const SHistoryCodeItem& rfHistoryCode = it_h->second;
                        SchemeDetail detail;
                        detail.nID = nIndex++; // no use. only used as key
                        detail.strLottery = apLottery->c_str();
                        detail.strIssue = rfHistoryCode.strIssue;
                        detail.strPlayKind = rfSubScheme.strPlayKind;
                        detail.strPlayName = rfSubScheme.strPlayName;
                        detail.nDWDPos = rfSubScheme.nDWDPos;
                        detail.nRoundTotal = rfSubScheme.nIssuesPerRound;
                        if (pLastSchemeDetail != NULL && !pLastSchemeDetail->bWin)
                        {
                            detail.nRoundIndex = (pLastSchemeDetail->nRoundIndex + 1) % detail.nRoundTotal;
                            if (detail.nRoundIndex == 0)
                                detail.strCode = lottery::GenereateCode(rfSubScheme);
                            else
                                detail.strCode = pLastSchemeDetail->strCode;
                        }
                        else
                        {
                            detail.nRoundIndex = 0;
                            detail.strCode = lottery::GenereateCode(rfSubScheme);
                        }
                        detail.nMerchantID = rfSubScheme.nMerchantID;
                        detail.nSchemeID = rfSubScheme.nSchemeID;
                        detail.nSubSchemeID = rfSubScheme.nID;
                        detail.strOpenCode = rfHistoryCode.strCode;
                        detail.bWin = lottery::CheckWin(detail);
                        mapSchemeDetails[detail.nID] = detail;
                        pLastSchemeDetail = &mapSchemeDetails[detail.nID];

                        // 统计子计划数据
                        rfSubScheme.nIssues++;
                        if (detail.nRoundIndex == 0) rfSubScheme.nRounds++;
                        lottery::StatisticSubScheme(rfSubScheme, detail);
                    }
                    // 生成当前期的计划详情
                    {
                        SchemeDetail detail;
                        detail.nID = nIndex++; // no use. only used as key
                        detail.strLottery = apLottery->c_str();
                        detail.strIssue = rfCurIssue;
                        detail.strPlayKind = rfSubScheme.strPlayKind;
                        detail.strPlayName = rfSubScheme.strPlayName;
                        detail.nDWDPos = rfSubScheme.nDWDPos;
                        detail.nRoundTotal = rfSubScheme.nIssuesPerRound;
                        if (pLastSchemeDetail != NULL && !pLastSchemeDetail->bWin)
                        {
                            detail.nRoundIndex = (pLastSchemeDetail->nRoundIndex + 1) % detail.nRoundTotal;
                            if (detail.nRoundIndex == 0)
                                detail.strCode = lottery::GenereateCode(rfSubScheme);
                            else
                                detail.strCode = pLastSchemeDetail->strCode;
                        }
                        else
                        {
                            detail.nRoundIndex = 0;
                            detail.strCode = lottery::GenereateCode(rfSubScheme);
                        }
                        detail.nMerchantID = rfSubScheme.nMerchantID;
                        detail.nSchemeID = rfSubScheme.nSchemeID;
                        detail.nSubSchemeID = rfSubScheme.nID;
                        mapSchemeDetails[detail.nID] = detail;
                        pLastSchemeDetail = &mapSchemeDetails[detail.nID];

                        INFO_LOG(g_pLogger, "=== create new scheme detail[merchant_id:%d, scheme_id:%d, sub_scheme_id:%d, lottery:%s, issue:%d] ===",
                                 detail.nMerchantID, detail.nSchemeID, detail.nSubSchemeID, detail.strLottery.c_str(), detail.strIssue.c_str());

                        rfSubScheme.nIssues++;
                        if (detail.nRoundIndex == 0) rfSubScheme.nRounds++;
                    }
                    g_dbWrapper.AddSchemeDetails(mapSchemeDetails);
                    g_dbWrapper.UpdateSubSchemeStatistic(rfSubScheme);

                    lstSubSchemeUpdated.push_back(rfSubScheme);

                    INFO_LOG(g_pLogger, "=== update sub_scheme statistic[merchant_id:%d, scheme_id:%d, sub_scheme_id:%d, name:%s, lottery:%s, play_kind:%s, play_name:%s, issues:%d,"
                             " win_rounds/rounds:%d/%d, comb_win_rounds/max_comb_win_rounds:%d/%d, comb_loss_rounds/max_comb_loss_rounds:%d/%d, accuracy:%0.2f%%] ===",
                             rfSubScheme.nMerchantID,
                             rfSubScheme.nSchemeID,
                             rfSubScheme.nID,
                             rfSubScheme.strName.c_str(),
                             rfSubScheme.strLottery.c_str(),
                             rfSubScheme.strPlayKind.c_str(),
                             rfSubScheme.strPlayName.c_str(),
                             rfSubScheme.nIssues,
                             rfSubScheme.nWinRounds, rfSubScheme.nRounds,
                             rfSubScheme.nCombWinRounds, rfSubScheme.nMaxCombWinRounds,
                             rfSubScheme.nCombLossRounds, rfSubScheme.nMaxCombLossRounds,
                             rfSubScheme.dAccuracy);
                }

                MQTTSchemeNotify(lstSubSchemeUpdated, rfCurIssue.c_str());
            }
            break;
    }
    return 0;
}

void CRoutine::Routine()
{
    static long tm = 0;
    long lnow = Time::GetCurDateTime().GetDateTime();
    if (lnow - tm < ROUTINE_INTERVAL) return;

    INFO_LOG(g_pLogger, "=== getting open code... last perform time: %ld ===", lnow);
    tm = lnow;

    string_list lostterys;
    g_dbWrapper.GetSchemeLotterys(lostterys);

    int issues = (m_mapAllHistoryCode.empty()) ? 50 : 1;
    for (string_list::iterator it = lostterys.begin(); it != lostterys.end(); it++)
        g_netManager.GetHistoryCodeRequest(it->c_str(), issues);
}

void CRoutine::MQTTSchemeNotify(const list<SubScheme>& lstSubSchemeUpdated, _lpcstr issue)
{
    map<int, Scheme> mapSchemes;
    for (list<SubScheme>::const_iterator it = lstSubSchemeUpdated.begin(); it != lstSubSchemeUpdated.end(); it++)
    {
        Scheme sc;
        sc.nMerchantID = it->nMerchantID;
        sc.strLottery = it->strLottery;
        sc.nID = it->nSchemeID;
        mapSchemes[sc.nID] = sc;
    }
    for (map<int, Scheme>::iterator it = mapSchemes.begin(); it != mapSchemes.end(); it++)
    {
        const Scheme& sc = it->second;

        CJsonValue jv;
        jv["merchant"] = sc.nMerchantID;
        jv["lottery"] = sc.strLottery;
        jv["issue"] = issue;
        jv["scheme"] = sc.nID;
        g_mqttClient.SendNotify(enum_str(mqtt_notify, mqtt_notify_scheme_updated).c_str(), CJsonWrapper::Dumps(jv).c_str(), false);
    }
}
