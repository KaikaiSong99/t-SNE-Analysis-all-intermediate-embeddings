
// Its own header file:
#include "DimensionSelectionWidget.h"

#include "DimensionSelectionHolder.h"
#include "DimensionSelectionItemModel.h"
#include "ModelResetter.h"
#include "PointsPlugin.h"

// Qt header files:
#include <QAbstractEventDispatcher>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QFileDialog>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QString>
#include <QTime>

// Standard C++ header files:
#include <algorithm>
#include <cmath>
#include <execution>
#include <limits>
#include <vector>

// Header file, generated by Qt User Interface Compiler:
#include <ui_DimensionSelectionWidget.h>


namespace hdps
{
    namespace
    {
        class ProxyModel final : public QSortFilterProxyModel
        {
            const DimensionSelectionHolder& _holder;

        public:
            explicit ProxyModel(const DimensionSelectionHolder& holder)
                :
                _holder(holder)
            {
            }
        private:
            bool lessThan(const QModelIndex &modelIndex1, const QModelIndex &modelIndex2) const override
            {
                const auto column = modelIndex1.column();

                if ((column == modelIndex2.column())  && (column >= 0) && (column < static_cast<int>(DimensionSelectionItemModel::ColumnEnum::count) ))
                {
                    const auto row1 = modelIndex1.row();
                    const auto row2 = modelIndex2.row();

                    switch ( static_cast<DimensionSelectionItemModel::ColumnEnum>(column))
                    {
                    case DimensionSelectionItemModel::ColumnEnum::Name:
                    {
                        return _holder.lessThanName(row1, row2);
                    }
                    case DimensionSelectionItemModel::ColumnEnum::Mean:
                    {
                        if (_holder._statistics.empty())
                        {
                            break;
                        }
                        return _holder._statistics[row1].mean < _holder._statistics[row2].mean;
                    }
                    case DimensionSelectionItemModel::ColumnEnum::MeanOfZeroValues:
                    {
                        if (_holder._statistics.empty())
                        {
                            break;
                        }
                        return _holder._statistics[row1].meanOfNonZero < _holder._statistics[row2].meanOfNonZero;
                    }
                    }
                }
                return QSortFilterProxyModel::lessThan(modelIndex1, modelIndex2);
            }
        };



        QString getSelectionFileFilter()
        {
            return QObject::tr("Text files (*.txt);;All files (*.*)");
        }


        template <typename T>
        void connectPushButton(QPushButton& pushButton, const T slot)
        {
            QObject::connect(&pushButton, &QPushButton::clicked, [slot, &pushButton]
            {
                try
                {
                    slot();
                }
                catch (const std::exception& stdException)
                {
                    qCritical()
                        << "Exception \""
                        << typeid(stdException).name()
                        << "\" on "
                        << pushButton.text()
                        << " button click: "
                        << stdException.what();
                }
            });
        }


        void readSelectionFromFile(QFile& file, DimensionSelectionHolder& holder)
        {
            holder.disableAllDimensions();

            while (!file.atEnd())
            {
                const auto timmedLine = file.readLine().trimmed();

                if (!timmedLine.isEmpty())
                {
                    const auto name = QString::fromUtf8(timmedLine);

                    if (!holder.tryToEnableDimensionByName(name))
                    {
                        qWarning() << "Failed to select dimension (name not found): " << name;
                    }
                }
            }
        }


        void readSelectionFromFile(const QString& fileName, QAbstractItemModel* const itemModel, DimensionSelectionHolder& holder)
        {
            if (!fileName.isEmpty())
            {
                QFile file(fileName);

                if (file.open(QIODevice::ReadOnly | QIODevice::Text))
                {
                    const ModelResetter modelResetter(itemModel);
                    readSelectionFromFile(file, holder);
                }
                else
                {
                    qCritical() << "Load failed to open file: " << fileName;
                }
            }
        }


        void writeSelectionToFile(const QString& fileName, const DimensionSelectionHolder& holder)
        {
            if (!fileName.isEmpty())
            {
                QFile file(fileName);

                if (file.open(QIODevice::WriteOnly | QIODevice::Text))
                {
                    const auto numberOfDimensions = holder.getNumberOfDimensions();

                    for (std::size_t i{}; i < numberOfDimensions; ++i)
                    {
                        if (holder.isDimensionEnabled(i))
                        {
                            file.write(holder.getName(i).toUtf8());
                            file.write("\n");
                        }
                    }
                }
                else
                {
                    qCritical() << "Save failed to open file: " << fileName;
                }
            }
        }

        void CopyModelToClipboard(const QAbstractItemModel& model)
        {
            
            QString result;
            const auto quote = QChar::fromLatin1('"');
            const auto tabChar = QChar::fromLatin1('\t');
            const auto linebreakChar = QChar::fromLatin1('\n');

            const auto columnCount = model.columnCount();

            for (int columnIndex = 0; columnIndex < columnCount; ++columnIndex)
            {
                if (columnIndex != 0)
                {
                    result += tabChar;
                }

                QString header = model.headerData(columnIndex, Qt::Horizontal).toString();
                result += quote + header + quote;
            }
            result += linebreakChar;

            const auto rowCount = model.rowCount();

            for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex)
            {
                for (int columnIndex = 0; columnIndex < columnCount; ++columnIndex)
                {
                    if (columnIndex != 0)
                    {
                        result += tabChar;
                    }

                    result += model.data(model.index(rowIndex, columnIndex)).toString();
                }
                result += linebreakChar;
            }
            QApplication::clipboard()->setText(result);

        }

    }  // End of namespace.


    class DimensionSelectionWidget::Impl
    {
    public:
        const Ui_DimensionSelectionWidget _ui;

        DimensionSelectionHolder _holder;

        std::unique_ptr<DimensionSelectionItemModel> _dimensionSelectionItemModel;
        std::unique_ptr<ProxyModel> _proxyModel;

        QMetaObject::Connection m_awakeConnection;

        const PointsPlugin* _pointsPlugin = nullptr;


    private:
        void updateLabel()
        {
            const auto& holder = _holder;

            _ui.label->setText(QObject::tr("%1 available, %2 visible, %3 selected").
                arg(holder.getNumberOfDimensions()).
                arg(holder.getNumberOfDimensions()).
                arg(holder.getNumberOfSelectedDimensions()) );
        }
    public:
        Impl(QWidget& widget)
            :
            _ui([&widget]
        {
            Ui_DimensionSelectionWidget ui;
            ui.setupUi(&widget);
            return ui;
        }())
        {
            updateLabel();

            connectPushButton(*_ui.loadPushButton, [this, &widget]
            {
                const auto fileName = QFileDialog::getOpenFileName(&widget,
                    QObject::tr("Dimension selection"), {}, getSelectionFileFilter());
                readSelectionFromFile(fileName, _proxyModel.get(), _holder);
            });

            connectPushButton(*_ui.savePushButton, [this, &widget]
            {
                const auto fileName = QFileDialog::getSaveFileName(&widget,
                    QObject::tr("Dimension selection"), {}, getSelectionFileFilter());
                writeSelectionToFile(fileName, _holder);
            });

            connectPushButton(*_ui.computeStatisticsPushButton, [this]
            {
                const ModelResetter modelResetter(_proxyModel.get());

                auto& statistics = _holder._statistics;
                statistics.clear();

                if (_pointsPlugin != nullptr)
                {
                    QTime time;

                    time.start();
                    const auto& pointsPlugin = *_pointsPlugin;
                    const auto numberOfDimensions = pointsPlugin.getNumDimensions();
                    const auto numberOfPoints = pointsPlugin.getNumPoints();

                    constexpr static auto quiet_NaN = std::numeric_limits<double>::quiet_NaN();

                    if (numberOfPoints == 0)
                    {
                        statistics.resize(numberOfDimensions, { quiet_NaN, quiet_NaN, quiet_NaN });
                    }
                    else
                    {
                        statistics.resize(numberOfDimensions);
                        const auto* const statisticsData = statistics.data();

                        (void)std::for_each_n(std::execution::par_unseq, statistics.begin(), numberOfDimensions,
                            [statisticsData, &pointsPlugin, numberOfDimensions, numberOfPoints](auto& statisticsPerDimension)
                        {
                            thread_local const std::unique_ptr<double []> data(new double[numberOfPoints]);

                            {
                                const auto i = &statisticsPerDimension - statisticsData;

                                for (unsigned j{}; j < numberOfPoints; ++j)
                                {
                                    data[j] = pointsPlugin[j * numberOfDimensions + i];
                                }
                            }

                            double sum{};
                            unsigned numberOfNonZeroValues{};

                            for (unsigned j{}; j < numberOfPoints; ++j)
                            {
                                const auto value = data[j];

                                if (value != 0.0)
                                {
                                    sum += value;
                                    ++numberOfNonZeroValues;
                                }
                            }
                            const auto mean = sum / numberOfPoints;

                            double sumOfSquares{};

                            for (unsigned j{}; j < numberOfPoints; ++j)
                            {
                                const auto value = data[j] - mean;
                                sumOfSquares += value * value;
                            }

                            static_assert(quiet_NaN != quiet_NaN);

                            statisticsPerDimension = StatisticsPerDimension
                            {
                            mean,
                            (numberOfNonZeroValues == 0) ? quiet_NaN : (sum / numberOfNonZeroValues),
                            std::sqrt(sumOfSquares / (numberOfPoints - 1))
                            };
                        });
                    }
                    qDebug()
                        << " Duration: " << time.elapsed() << " microsecond(s)";
                }

            });

            assert(_ui.tableView->contextMenuPolicy() == Qt::CustomContextMenu);

            connect(_ui.tableView, &QWidget::customContextMenuRequested, [this](const QPoint&)
            {
                if (_proxyModel != nullptr)
                {
                    QMenu menu;

                    menu.addAction(tr("Copy table"), [this] {CopyModelToClipboard(*_proxyModel); });
                    menu.exec(QCursor::pos());
                }
            }
            );


            // Reset the view "on idle".
            m_awakeConnection = connect(
                QAbstractEventDispatcher::instance(),
                &QAbstractEventDispatcher::awake,
                [this]
            {
                updateLabel();
            });
        }


        ~Impl()
        {
            disconnect(m_awakeConnection);
        }


        void setDimensions(
            const unsigned numberOfDimensions, const std::vector<QString>& names)
        {
            if (names.size() == numberOfDimensions)
            {
                _holder = DimensionSelectionHolder(
                    names.data(),
                    numberOfDimensions);

            }
            else
            {
                assert(names.empty());
                _holder = DimensionSelectionHolder(numberOfDimensions);
            }
            auto dimensionSelectionItemModel = std::make_unique<DimensionSelectionItemModel>(_holder);
            auto proxyModel = std::make_unique<ProxyModel>(_holder);
            proxyModel->setSourceModel(&*dimensionSelectionItemModel);
            _ui.tableView->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
            _ui.tableView->setModel(&*proxyModel);
            _proxyModel = std::move(proxyModel);
            _dimensionSelectionItemModel = std::move(dimensionSelectionItemModel);
        }


        std::vector<bool> getEnabledDimensions() const
        {
            return _holder.getEnabledDimensions();
        }


        void dataChanged(const PointsPlugin& pointsPlugin)
        {
            _pointsPlugin = &pointsPlugin;
            const auto numberOfDimensions = pointsPlugin.getNumDimensions();
            setDimensions(numberOfDimensions, pointsPlugin.getDimensionNames());
        }
    };


    DimensionSelectionWidget::DimensionSelectionWidget()
        :
        _pImpl(std::make_unique<Impl>(*this))
    {
    }

    DimensionSelectionWidget::~DimensionSelectionWidget() = default;


    void DimensionSelectionWidget::dataChanged(const PointsPlugin& pointsPlugin)
    {
        _pImpl->dataChanged(pointsPlugin);
    }

    std::vector<bool> DimensionSelectionWidget::getEnabledDimensions() const
    {
        return _pImpl->getEnabledDimensions();
    }

} // namespace hdps
