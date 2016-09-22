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

}

DICOMListingTable::~DICOMListingTable()
{

}

void DICOMListingTable::setData(const std::vector<Registry> &reg)
{
  this->clear();
  this->setSortingEnabled(false);

  this->setRowCount(reg.size());
  this->setColumnCount(4);
  this->setHorizontalHeaderItem(0, new QTableWidgetItem("Series Number"));
  this->setHorizontalHeaderItem(1, new QTableWidgetItem("Description"));
  this->setHorizontalHeaderItem(2, new QTableWidgetItem("Dimensions"));
  this->setHorizontalHeaderItem(3, new QTableWidgetItem("Number of Images"));


  for(size_t i = 0; i < reg.size(); i++)
    {
    Registry r = reg[i];

    // Series number
    QTableWidgetItem *iSN = new QTableWidgetItem();
    iSN->setData(Qt::EditRole, r["SeriesNumber"][0]);
    iSN->setData(Qt::UserRole, from_utf8(r["SeriesId"][""]));
    this->setItem(i, 0, iSN);

    // Strings
    this->setItem(i, 1, new QTableWidgetItem(r["SeriesDescription"][""]));
    this->setItem(i, 2, new QTableWidgetItem(r["Dimensions"][""]));

    // Number of images
    QTableWidgetItem *iNI = new QTableWidgetItem();
    iNI->setData(Qt::EditRole, r["NumberOfImages"][0]);
    this->setItem(i, 3, iNI);
    }

  this->resizeColumnsToContents();
  this->resizeRowsToContents();

  // If only one sequence selected, pick it
  if(reg.size() == 1)
    {
    this->selectRow(0);
    }

  // Sort by number
  this->setSortingEnabled(true);
  this->sortByColumn(0, Qt::AscendingOrder);
}

