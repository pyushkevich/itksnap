#ifndef DICOMLISTINGTABLE_H
#define DICOMLISTINGTABLE_H

#include <QWidget>
#include <QTableWidget>
#include <vector>

class Registry;

class DICOMListingTable : public QTableWidget
{
  Q_OBJECT

public:
  DICOMListingTable(QWidget *parent = 0);
  ~DICOMListingTable();

  void setData(const std::vector<Registry> &reg);
};

#endif // DICOMLISTINGTABLE_H
