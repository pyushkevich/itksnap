#include "DICOMListingTable.h"
#include "QHeaderView"
#include "Registry.h"
#include "SNAPQtCommon.h"

DICOMListingTable::DICOMListingTable(QWidget *parent)
  : QTableWidget(parent)
{
  this->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->setSelectionMode(QAbstractItemView::SingleSelection);
  this->setAlternatingRowColors(true);
  this->setEditTriggers(QAbstractItemView::NoEditTriggers);
  this->verticalHeader()->hide();

  this->setSortingEnabled(true);
}

DICOMListingTable::~DICOMListingTable()
{

}

void DICOMListingTable::setData(const std::vector<Registry> &reg)
{
  this->setRowCount(reg.size());
  this->setColumnCount(4);
  this->setHorizontalHeaderItem(0, new QTableWidgetItem("Series Number"));
  this->setHorizontalHeaderItem(1, new QTableWidgetItem("Description"));
  this->setHorizontalHeaderItem(2, new QTableWidgetItem("Dimensions"));
  this->setHorizontalHeaderItem(3, new QTableWidgetItem("Number of Images"));

  for(size_t i = 0; i < reg.size(); i++)
    {
    Registry r = reg[i];
    this->setItem(i, 0, new QTableWidgetItem(r["SeriesNumber"][""]));
    this->setItem(i, 1, new QTableWidgetItem(r["SeriesDescription"][""]));
    this->setItem(i, 2, new QTableWidgetItem(r["Dimensions"][""]));
    this->setItem(i, 3, new QTableWidgetItem(r["NumberOfImages"][""]));

    this->item(i, 0)->setData(Qt::UserRole, from_utf8(r["SeriesId"][""]));
    }

  this->resizeColumnsToContents();
  this->resizeRowsToContents();

  // If only one sequence selected, pick it
  if(reg.size() == 1)
    {
    this->selectRow(0);
    }
}

