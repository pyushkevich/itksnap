#ifndef DEEPLEARNINGCONNECTIONSTATUSCOUPLINGTRAITS_H
#define DEEPLEARNINGCONNECTIONSTATUSCOUPLINGTRAITS_H

#include "DeepLearningSegmentationModel.h"
#include <QLabel>
#include <QtWidgetCoupling.h>


/**
 * Traits for mapping status codes to a label
 */
inline std::pair<QString, QString> GetDLSConnectionStatusColorAndText(dls_model::ConnectionStatus value)
{
  switch (value.status)
  {
    case dls_model::CONN_NO_SERVER:
      return { "darkred", "Server not configured" };

    case dls_model::CONN_TUNNEL_ESTABLISHING:
      return { "black", "Opening tunnel to SSH server ..." };

    case dls_model::CONN_TUNNEL_FAILED:
      return { "darkred", QString("SSH tunnel failure: %1").arg(QString::fromStdString(value.error_message)) };

    case dls_model::CONN_CHECKING:
      return { "black", "Establishing connection ..." };

    case dls_model::CONN_NOT_CONNECTED:
      return { "darkred", QString("Not connected: %1").arg(QString::fromStdString(value.error_message)) };

    case dls_model::CONN_LOCAL_SERVER_STARTING:
      return { "goldenrod", "Starting local server ..." };

    case dls_model::CONN_LOCAL_SERVER_FAILED:
      return { "darkred", "Failed to start local server." };

    case dls_model::CONN_CONNECTED:
      return { "darkgreen", QString("Connected, server version: %1").arg(QString::fromStdString(value.server_version)) };

    default:
      return { "gray", "Unknown status" };
  }
};


/**
 * Traits for mapping status codes to a label
 */
class MiniConnectionStatusQLabelValueTraits
  : public WidgetValueTraitsBase<dls_model::ConnectionStatus, QLabel *>
{
public:
  typedef dls_model::ConnectionStatus TAtomic;

  virtual TAtomic GetValue(QLabel *w)
  {
    return dls_model::ConnectionStatus();
  }

  virtual void SetValue(QLabel *w, const TAtomic &value)
  {
    QString color, text;
    std::tie(color, text) = GetDLSConnectionStatusColorAndText(value);
    w->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; opacity: 0.5; }").arg(color));
    w->setToolTip(QString("Deep learning server status: %1").arg(text));
  }
};

#endif // DEEPLEARNINGCONNECTIONSTATUSCOUPLINGTRAITS_H
