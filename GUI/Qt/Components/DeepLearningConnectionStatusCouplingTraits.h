#ifndef DEEPLEARNINGCONNECTIONSTATUSCOUPLINGTRAITS_H
#define DEEPLEARNINGCONNECTIONSTATUSCOUPLINGTRAITS_H

#include "DeepLearningSegmentationModel.h"
#include <QLabel>
#include <QtWidgetCoupling.h>
#include <QCoreApplication>

/**
 * Traits for mapping status codes to a label
 */
inline std::pair<QString, QString> GetDLSConnectionStatusColorAndText(dls_model::ConnectionStatus value)
{
  auto tr = [](auto a) { return QCoreApplication::translate("DeepLearningSegmentationModel", a); };

  switch (value.status)
  {
    case dls_model::CONN_NO_SERVER:
      return { "darkred", tr("Server not configured") };

    case dls_model::CONN_TUNNEL_ESTABLISHING:
      return { "black", tr("Opening tunnel to SSH server ...") };

    case dls_model::CONN_TUNNEL_FAILED:
      return { "darkred", tr("SSH tunnel failure: %1").arg(QString::fromStdString(value.message)) };

    case dls_model::CONN_CHECKING:
      return { "black", tr("Establishing connection ...") };

    case dls_model::CONN_NOT_CONNECTED:
      return { "darkred", tr("Not connected: %1").arg(QString::fromStdString(value.message)) };

    case dls_model::CONN_LOCAL_SERVER_STARTING:
      return { "goldenrod", tr("Starting local server ...") };

    case dls_model::CONN_LOCAL_SERVER_FAILED:
      return { "darkred", tr("Failed to start local server.") };

    case dls_model::CONN_INCOMPATIBLE:
      return
      {
        "darkred", tr("Server version %1 is too old; %2+ required")
                     .arg(QString::fromStdString(value.message),
                          DeepLearningSegmentationModel::MINIMUM_SERVER_VERSION)
      };

    case dls_model::CONN_CONNECTED:
      return { "darkgreen", QString("Connected, server version: %1").arg(QString::fromStdString(value.message)) };

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
