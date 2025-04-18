#ifndef DEEPLEARNINGCONNECTIONSTATUSCOUPLINGTRAITS_H
#define DEEPLEARNINGCONNECTIONSTATUSCOUPLINGTRAITS_H

#include "DeepLearningSegmentationModel.h"
#include <QLabel>
#include <QtWidgetCoupling.h>

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
    QString color;
    switch(value.status)
    {
      case dls_model::CONN_NO_SERVER:
        color = "darkred";
        break;
      case dls_model::CONN_TUNNEL_ESTABLISHING:
      case dls_model::CONN_CHECKING:
        color = "black";
        break;
      case dls_model::CONN_NOT_CONNECTED:
      case dls_model::CONN_TUNNEL_FAILED:
        color = "darkred";
        break;
      case dls_model::CONN_CONNECTED:
        color = "darkgreen";
        break;
    }
    w->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; opacity: 0.5; }").arg(color));
  }
};

#endif // DEEPLEARNINGCONNECTIONSTATUSCOUPLINGTRAITS_H
