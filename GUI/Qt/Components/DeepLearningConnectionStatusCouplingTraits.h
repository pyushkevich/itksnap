#ifndef DEEPLEARNINGCONNECTIONSTATUSCOUPLINGTRAITS_H
#define DEEPLEARNINGCONNECTIONSTATUSCOUPLINGTRAITS_H

#include "DeepLearningSegmentationModel.h"
#include <QLabel>
#include <QtWidgetCoupling.h>
#include <QCoreApplication>

/**
 * The color and text that describe a DLS connection status. A class so that
 * lupdate finds the tr() calls (it does not see calls to a local lambda).
 */
class DLSConnectionStatusText
{
  Q_DECLARE_TR_FUNCTIONS(DLSConnectionStatusText)

public:
  static std::pair<QString, QString> Get(dls_model::ConnectionStatus value);
};

inline std::pair<QString, QString> GetDLSConnectionStatusColorAndText(dls_model::ConnectionStatus value)
{
  return DLSConnectionStatusText::Get(value);
}

inline std::pair<QString, QString> DLSConnectionStatusText::Get(dls_model::ConnectionStatus value)
{
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
                            DeepLearningSegmentationModel::MINIMUM_SERVER_VERSION) +
                     " " + tr("Upgrade it with: pip install -U itksnap-dls")
      };

    case dls_model::CONN_CONNECTED:
      return { "darkgreen", tr("Connected, server version: %1").arg(QString::fromStdString(value.message)) };

    default:
      return { "gray", tr("Unknown status") };
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
    w->setToolTip(DLSConnectionStatusText::tr("Deep learning server status: %1").arg(text));
  }
};

#endif // DEEPLEARNINGCONNECTIONSTATUSCOUPLINGTRAITS_H
