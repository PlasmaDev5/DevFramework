#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/ThreadUtils.h>

class plTelemetryThread;

plTelemetry::plEventTelemetry plTelemetry::s_TelemetryEvents;
plUInt32 plTelemetry::s_uiApplicationID = 0;
plUInt32 plTelemetry::s_uiServerID = 0;
plUInt16 plTelemetry::s_uiPort = 1040;
bool plTelemetry::s_bConnectedToServer = false;
bool plTelemetry::s_bConnectedToClient = false;
bool plTelemetry::s_bAllowNetworkUpdate = true;
plTime plTelemetry::s_PingToServer;
plString plTelemetry::s_sServerName;
plString plTelemetry::s_sServerIP;
plTelemetry::ConnectionMode plTelemetry::s_ConnectionMode = plTelemetry::None;
plMap<plUInt64, plTelemetry::MessageQueue> plTelemetry::s_SystemMessages;

void plTelemetry::UpdateServerPing()
{

}

void plTelemetry::UpdateNetwork()
{

}

void plTelemetry::SetServerName(plStringView sName)
{
  if (s_ConnectionMode == ConnectionMode::Client)
    return;

  if (s_sServerName == sName)
    return;

  s_sServerName = sName;

  SendServerName();
}

void plTelemetry::SendServerName()
{
  if (!IsConnectedToOther())
    return;

  char data[48];
  plStringUtils::Copy(data, PL_ARRAY_SIZE(data), s_sServerName.GetData());

  Broadcast(plTelemetry::Reliable, 'PLBC', 'NAME', data, PL_ARRAY_SIZE(data));
}

plResult plTelemetry::RetrieveMessage(plUInt32 uiSystemID, plTelemetryMessage& out_message)
{
  if (s_SystemMessages[uiSystemID].m_IncomingQueue.IsEmpty())
    return PL_FAILURE;

  PL_LOCK(GetTelemetryMutex());

  // check again while inside the lock
  if (s_SystemMessages[uiSystemID].m_IncomingQueue.IsEmpty())
    return PL_FAILURE;

  out_message = s_SystemMessages[uiSystemID].m_IncomingQueue.PeekFront();
  s_SystemMessages[uiSystemID].m_IncomingQueue.PopFront();

  return PL_SUCCESS;
}

void plTelemetry::InitializeAsServer()
{
  plLog::SeriousWarning("Enet is not compiled into this build, plTelemetry::InitializeAsServer() will be ignored.");
}

plResult plTelemetry::InitializeAsClient(plStringView sConnectTo0)
{
  PL_IGNORE_UNUSED(sConnectTo0);
  plLog::SeriousWarning("Enet is not compiled into this build, plTelemetry::InitializeAsClient() will be ignored.");

  return PL_FAILURE;
}

plResult plTelemetry::OpenConnection(ConnectionMode Mode, plStringView sConnectTo)
{
  PL_IGNORE_UNUSED(Mode);
  PL_IGNORE_UNUSED(sConnectTo);
  plLog::SeriousWarning("Enet is not compiled into this build, plTelemetry::OpenConnection() will be ignored.");
  return PL_FAILURE;
}

void plTelemetry::Transmit(TransmitMode tm, const void* pData, plUInt32 uiDataBytes)
{
  PL_IGNORE_UNUSED(tm);
  PL_IGNORE_UNUSED(pData);
  PL_IGNORE_UNUSED(uiDataBytes);
}

void plTelemetry::Send(TransmitMode tm, plUInt32 uiSystemID, plUInt32 uiMsgID, const void* pData, plUInt32 uiDataBytes)
{
  PL_IGNORE_UNUSED(tm);
  PL_IGNORE_UNUSED(uiSystemID);
  PL_IGNORE_UNUSED(uiMsgID);
  PL_IGNORE_UNUSED(pData);
  PL_IGNORE_UNUSED(uiDataBytes);
}

void plTelemetry::Send(TransmitMode tm, plUInt32 uiSystemID, plUInt32 uiMsgID, plStreamReader& Stream, plInt32 iDataBytes)
{
  PL_IGNORE_UNUSED(tm);
  PL_IGNORE_UNUSED(uiSystemID);
  PL_IGNORE_UNUSED(uiMsgID);
  PL_IGNORE_UNUSED(Stream);
  PL_IGNORE_UNUSED(iDataBytes);
}

void plTelemetry::CloseConnection()
{
}
