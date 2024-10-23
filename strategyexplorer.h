#ifndef STRATEGYEXPLORER_H
#define STRATEGYEXPLORER_H

#include <QDialog>
#include <QTimer>
#include <QMouseEvent>
#include <QEvent>
#include <QMouseEvent>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>

#include "include/runtime/qsolverjob.h"
#include "QItemSelection"
#include "include/ui/worditemdelegate.h"
#include "include/ui/tablestrategymodel.h"
#include "include/ui/strategyitemdelegate.h"
#include "include/ui/detailwindowsetting.h"
#include "include/Card.h"
#include "include/ui/detailviewermodel.h"
#include "include/ui/detailitemdelegate.h"
#include "include/ui/roughstrategyviewermodel.h"
#include "include/ui/roughstrategyitemdelegate.h"
#include "include/nodes/GameTreeNode.h"
#include "include/nodes/ActionNode.h"
#include "include/nodes/ChanceNode.h"
#include "include/nodes/TerminalNode.h"
#include "include/nodes/ShowdownNode.h"

namespace Ui {
class StrategyExplorer;
}

class StrategyExplorer : public QDialog
{
    Q_OBJECT

public:
    explicit StrategyExplorer(QWidget *parent = 0,QSolverJob * qSolverJob=nullptr);
    ~StrategyExplorer();
    ActionNode* getCurrentNode();
    std::vector<GameActions> getNodeActions() {
        return nodeActions;
    }
    void setNodeActions(std::vector<GameActions> actions) {
        this->nodeActions = actions;
    }
    void setActionsLocked(std::vector<GameActions> actions) {
        this->actionsLocked = actions;
    }
    std::vector<GameActions> getActionsLocked() {
        return actionsLocked;
    }
    void clearActionsLocked() {
        actionsLocked.clear();
    }
    void addActionLocked(GameActions action);
    void removeActionLocked(GameActions action);

private:
    DetailWindowSetting detailWindowSetting;
    QTimer *timer;
    Ui::StrategyExplorer *ui;
    QSolverJob * qSolverJob;
    StrategyItemDelegate * delegate_strategy;
    TableStrategyModel * tableStrategyModel;
    DetailViewerModel * detailViewerModel;
    DetailItemDelegate * detailItemItemDelegate;
    RoughStrategyViewerModel * roughStrategyViewerModel;
    RoughStrategyItemDelegate * roughStrategyItemDelegate;
    vector<Card> cards;

    // Labels para mostrar los porcentajes
    QLabel *foldText;
    QLabel *callText;
    QLabel *raiseText;
    QLabel *betText;
    QLabel *checkText;

    // Sliders para las acciones
    QSlider *foldSlider;
    QSlider *callSlider;
    QSlider *raiseSlider;
    QSlider *betSlider;
    QSlider *checkSlider;

    // Labels para mostrar los porcentajes
    QLabel *foldLabel;
    QLabel *callLabel;
    QLabel *raiseLabel;
    QLabel *betLabel;
    QLabel *checkLabel;

    // Checkboxes para bloquear las acciones
    QCheckBox *foldCheckBox;
    QCheckBox *callCheckBox;
    QCheckBox *raiseCheckBox;
    QCheckBox *betCheckBox;
    QCheckBox *checkCheckBox;

    // PushButton para recalcular el árbol
    QPushButton* recalculateTreeButton;

    shared_ptr<GameTree> gameTree;

    void process_treeclick(TreeItem* treeitem);
    void process_board(TreeItem* treeitem);

    void updateLockedSliders();

    std::vector<GameActions> nodeActions;
    std::vector<GameActions> actionsLocked;

public slots:
    void item_expanded(const QModelIndex& index);
    void item_clicked(const QModelIndex& index);
    void selection_changed(const QItemSelection &selected,
                                            const QItemSelection &deselected);
    void toggleFoldLock(int state);
    void toggleCallLock(int state);
    void toggleRaiseLock(int state);
    void toggleCheckLock(int state);
    void toggleBetLock(int state);
    void resetOptionPercentages();
    void updateNodeOptions();

    void updateActionProbability(GameTreeNode::PokerActions action, int value, QLabel* label);
    void recalculateTree();
private slots:
    void on_turnCardBox_currentIndexChanged(int index);
    void on_riverCardBox_currentIndexChanged(int index);
    void update_second();
    void onMouseMoveEvent(int i,int j);
    void on_strategyModeButtom_clicked();
    void on_ipRangeButtom_clicked();
    void on_oopRangeButtom_clicked();
    void on_evModeButtom_clicked();
    void on_evOnlyModeButtom_clicked();
};

#endif // STRATEGYEXPLORER_H
