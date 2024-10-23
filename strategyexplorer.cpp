#include "strategyexplorer.h"
#include "ui_strategyexplorer.h"
#include "qstandarditemmodel.h"
#include <QBrush>
#include <QApplication>
#include <QComboBox>
#include <QColor>
#include "include/Card.h"

StrategyExplorer::StrategyExplorer(QWidget *parent,QSolverJob * qSolverJob) :
    QDialog(parent),
    ui(new Ui::StrategyExplorer)
{
    this->qSolverJob = qSolverJob;
    this->detailWindowSetting = DetailWindowSetting();
    ui->setupUi(this);
    /*
    QStandardItemModel* model = new QStandardItemModel();
    for (int row = 0; row < 4; ++row) {
         QStandardItem *item = new QStandardItem(QString("%1").arg(row) );
         model->appendRow( item );
    }
    this->ui->gameTreeView->setModel(model);
    */
    // Initial Game Tree preview panel
    this->ui->gameTreeView->setTreeData(qSolverJob);
    connect(
                this->ui->gameTreeView,
                SIGNAL(expanded(const QModelIndex&)),
                this,
                SLOT(item_expanded(const QModelIndex&))
                );
    connect(
                this->ui->gameTreeView,
                SIGNAL(clicked(const QModelIndex&)),
                this,
                SLOT(item_clicked(const QModelIndex&))
                );

    // Initize strategy(rough) table
    this->tableStrategyModel = new TableStrategyModel(this->qSolverJob,this);
    this->ui->strategyTableView->setModel(this->tableStrategyModel);
    this->delegate_strategy = new StrategyItemDelegate(this->qSolverJob,&(this->detailWindowSetting),this);
    this->ui->strategyTableView->setItemDelegate(this->delegate_strategy);

    Deck* deck = this->qSolverJob->get_solver()->get_deck();
    int index = 0;
    QString board_qstring = QString::fromStdString(this->qSolverJob->board);
    for(Card one_card: deck->getCards()){
        if(board_qstring.contains(QString::fromStdString(one_card.toString())))continue;
        QString card_str_formatted = QString::fromStdString(one_card.toFormattedString());
        this->ui->turnCardBox->addItem(card_str_formatted);
        this->ui->riverCardBox->addItem(card_str_formatted);

        if(card_str_formatted.contains(QString::fromLocal8Bit("♦️")) ||
                card_str_formatted.contains(QString::fromLocal8Bit("♥️️"))){
            this->ui->turnCardBox->setItemData(0, QBrush(Qt::red),Qt::ForegroundRole);
            this->ui->riverCardBox->setItemData(0, QBrush(Qt::red),Qt::ForegroundRole);
        }else{
            this->ui->turnCardBox->setItemData(0, QBrush(Qt::black),Qt::ForegroundRole);
            this->ui->riverCardBox->setItemData(0, QBrush(Qt::black),Qt::ForegroundRole);
        }

        this->cards.push_back(one_card);
        index += 1;
    }
    if(this->qSolverJob->get_solver()->getGameTree()->getRoot()->getRound() == GameTreeNode::GameRound::FLOP){
        this->tableStrategyModel->setTrunCard(this->cards[0]);
        this->tableStrategyModel->setRiverCard(this->cards[1]);
        this->ui->riverCardBox->setCurrentIndex(1);
    }
    else if(this->qSolverJob->get_solver()->getGameTree()->getRoot()->getRound() == GameTreeNode::GameRound::TURN){
        this->tableStrategyModel->setRiverCard(this->cards[0]);
        this->ui->turnCardBox->clear();
    }
    else if(this->qSolverJob->get_solver()->getGameTree()->getRoot()->getRound() == GameTreeNode::GameRound::RIVER){
        this->ui->turnCardBox->clear();
        this->ui->riverCardBox->clear();
    }

    // Initize timer for strategy auto update
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update_second()));
    timer->start(1000);

    // On mouse event of strategy table
    connect(this->ui->strategyTableView,SIGNAL(itemMouseChange(int,int)),this,SLOT(onMouseMoveEvent(int,int)));

    // Initize Detail Viewer window
    this->detailViewerModel = new DetailViewerModel(this->tableStrategyModel,this);
    this->ui->detailView->setModel(this->detailViewerModel);
    this->detailItemItemDelegate = new DetailItemDelegate(&(this->detailWindowSetting),this);
    this->ui->detailView->setItemDelegate(this->detailItemItemDelegate);

    // Initize Rough Strategy Viewer
    this->roughStrategyViewerModel = new RoughStrategyViewerModel(this->tableStrategyModel,this);
    this->ui->roughStrategyView->setModel(this->roughStrategyViewerModel);
    this->roughStrategyItemDelegate = new RoughStrategyItemDelegate(&(this->detailWindowSetting),this);
    this->ui->roughStrategyView->setItemDelegate(this->roughStrategyItemDelegate);

    foldText = ui->title_fold;
    callText = ui->title_call;
    raiseText = ui->title_raise;
    betText = ui->title_bet;
    checkText = ui->title_check;

    foldSlider = ui->slider_fold;
    callSlider = ui->slider_call;
    raiseSlider = ui->slider_raise;
    betSlider = ui->slider_bet;
    checkSlider = ui->slider_check;

    foldLabel = ui->label_fold;
    callLabel = ui->label_call;
    raiseLabel = ui->label_raise;
    betLabel = ui->label_bet;
    checkLabel = ui->label_check;

    foldCheckBox = ui->checkBox_fold;
    callCheckBox = ui->checkBox_call;
    raiseCheckBox = ui->checkBox_raise;
    betCheckBox = ui->checkBox_bet;
    checkCheckBox = ui->checkBox_check;

    recalculateTreeButton = ui->recalculateTreeButton;

    // Conectar las señales a los slots
    connect(foldSlider, &QSlider::sliderMoved, this, [this](int value){ updateActionProbability(GameTreeNode::PokerActions::FOLD, value, foldLabel); });
    connect(callSlider, &QSlider::sliderMoved, this, [this](int value){ updateActionProbability(GameTreeNode::PokerActions::CALL, value, callLabel); });
    connect(raiseSlider, &QSlider::sliderMoved, this, [this](int value){ updateActionProbability(GameTreeNode::PokerActions::RAISE, value, raiseLabel); });
    connect(betSlider, &QSlider::sliderMoved, this, [this](int value){ updateActionProbability(GameTreeNode::PokerActions::BET, value, betLabel); });
    connect(checkSlider, &QSlider::sliderMoved, this, [this](int value){ updateActionProbability(GameTreeNode::PokerActions::CHECK, value, checkLabel); });

    // Conectar las señales de los checkboxes a los slots
    connect(foldCheckBox, &QCheckBox::checkStateChanged, this, &StrategyExplorer::toggleFoldLock);
    connect(callCheckBox, &QCheckBox::checkStateChanged, this, &StrategyExplorer::toggleCallLock);
    connect(raiseCheckBox, &QCheckBox::checkStateChanged, this, &StrategyExplorer::toggleRaiseLock);
    connect(betCheckBox, &QCheckBox::checkStateChanged, this, &StrategyExplorer::toggleBetLock);
    connect(checkCheckBox, &QCheckBox::checkStateChanged, this, &StrategyExplorer::toggleCheckLock);

    // Conectar las señales del botón para recalcular árbol
    connect(recalculateTreeButton, &QPushButton::clicked, this, &StrategyExplorer::recalculateTree);

    this->resetOptionPercentages();
}

StrategyExplorer::~StrategyExplorer()
{
    delete ui;
    delete this->delegate_strategy;
    delete this->tableStrategyModel;
    delete this->detailViewerModel;
    delete this->roughStrategyViewerModel;
    delete this->timer;
}

void StrategyExplorer::recalculateTree()
{
    GameTreeNode* currentNode = this->getCurrentNode();
    if (currentNode) {
        std::map<ActionType, double> newProbabilities = {
            {ActionType::Fold, foldSlider->value()},  // 20% de probabilidad de Fold por defecto
            {ActionType::Call, callSlider->value()},  // 50% de probabilidad de Call por defecto
            {ActionType::Raise, raiseSlider->value()},  // 30% de probabilidad de Raise por defecto
            {ActionType::Bet, betSlider->value()},  // 60% de probabilidad de Bet por defecto
            {ActionType::Check, checkSlider->value()}  // 40% de probabilidad de Check por defecto
        };
        // Actualizar las probabilidades de acción en todo el árbol
        //gameTree->updateActionProbabilities(rootNode, rootNode->getActionProbabilities(), gameTree->getActionLocked());
        this->qSolverJob->get_solver()->getGameTree()->updateActionProbabilities(currentNode, newProbabilities, this->qSolverJob->get_solver()->getGameTree()->getActionLocked());

        // Recalcular las estrategias para el subárbol actual
        std::shared_ptr<PCfrSolver> pcfrsolver = std::dynamic_pointer_cast<PCfrSolver>(this->qSolverJob->ps_holdem.get_solver()); // Casting a PCfrSolver*
        if (pcfrsolver) {
            if (currentNode->getType() == GameTreeNode::ACTION) {
                ActionNode* actionNode = dynamic_cast<ActionNode*>(currentNode);
                for (auto const& [action, probability] : actionNode->getActionProbabilities()) {
                    qDebug() << "Nuevas probabilidades: " << probability;
                }
            }

            pcfrsolver->retrain(currentNode);
        }

        // Actualizar la visualización del árbol
        emit this->ui->gameTreeView->tree_model->dataChanged(QModelIndex(), QModelIndex());
        this->ui->gameTreeView->update();

        //this->tableStrategyModel->updateStrategyData();
        //this->ui->strategyTableView->viewport()->update();
        //this->roughStrategyViewerModel->onchanged();
        //this->ui->roughStrategyView->triger_resize();
        //this->ui->roughStrategyView->viewport()->update();
    } else {
        // Manejar el caso en que no se pueda obtener el nodo raíz
        // ...
    }
}

void StrategyExplorer::updateActionProbability(GameTreeNode::PokerActions pokerAction, int value, QLabel* label) {

    double probability = value / 100.0;
    label->setText(QString::number(probability * 100) + "%");

    GameActions action;
    switch (pokerAction) {
        case GameTreeNode::PokerActions::BET:
            action = GameActions(pokerAction,0);
            break;
        case GameTreeNode::PokerActions::RAISE:
            action = GameActions(pokerAction,0);
            break;
        default:
            action = GameActions(pokerAction,-1);
            break;
    }

    // 1. Calcular la suma de las probabilidades de TODAS las acciones
    float sumTotal = foldSlider->value() + callSlider->value() + raiseSlider->value() + checkSlider->value() + betSlider->value();
    float difSum = 100 - sumTotal;
    std::vector<GameActions> actionsUnlocked = this->getNodeActions();
    difSum = 100 - sumTotal;
    for (const GameActions& nodeAction : this->getNodeActions()) {
        GameActions nodeActionCopy = nodeAction;
        for (const GameActions& actionLocked : this->getActionsLocked()) {
            GameActions actionLockedCopy = actionLocked;
            if (actionLockedCopy.getAction() == nodeActionCopy.getAction()) {
                std::vector<GameActions> temp = actionsUnlocked;
                // Utilizar std::find_if con una función lambda para buscar el elemento
                auto it = std::find_if(temp.begin(), temp.end(),
                                       [&nodeActionCopy](const GameActions& a) {
                                           GameActions actionCopy = nodeActionCopy; // Crear una copia del objeto GameActions
                                           GameActions aCopy = a; // Crear una copia del objeto GameActions
                                           return aCopy.getAction() == actionCopy.getAction();
                                       });

                // Si se encuentra el elemento, eliminarlo del vector
                if (it != temp.end()) {
                    temp.erase(it);
                    actionsUnlocked = temp;
                }
            }
        }
    }
    // 3. Las acciones disponibles
    bool isUpdated = false;
    for (GameActions& actionUnlocked : actionsUnlocked) {
        if (!isUpdated) {
            GameActions actionCopy = action;
            if (actionUnlocked.getAction() != actionCopy.getAction()) {
                switch (actionUnlocked.getAction()) {
                case GameTreeNode::PokerActions::FOLD:
                    foldSlider->setValue(foldSlider->value() + difSum);
                    foldLabel->setText(QString::number(foldSlider->value() + difSum, 'f', 1) + "%");
                    isUpdated = true;
                    break;
                case GameTreeNode::PokerActions::CALL:
                    callSlider->setValue(callSlider->value() + difSum);
                    callLabel->setText(QString::number(callSlider->value() + difSum, 'f', 1) + "%");
                    isUpdated = true;
                    break;
                case GameTreeNode::PokerActions::RAISE:
                    raiseSlider->setValue(raiseSlider->value() + difSum);
                    raiseLabel->setText(QString::number(raiseSlider->value() + difSum, 'f', 1) + "%");
                    isUpdated = true;
                    break;
                case GameTreeNode::PokerActions::BET:
                    betSlider->setValue(betSlider->value() + difSum);
                    betLabel->setText(QString::number(betSlider->value() + difSum, 'f', 1) + "%");
                    isUpdated = true;
                    break;
                case GameTreeNode::PokerActions::CHECK:
                    checkSlider->setValue(checkSlider->value() + difSum);
                    checkLabel->setText(QString::number(checkSlider->value() + difSum, 'f', 1) + "%");
                    isUpdated = true;
                    break;
                default: break;
                }
            }
        }
    }
}

void StrategyExplorer::resetOptionPercentages() {
    foldText->setVisible(false);
    foldSlider->setEnabled(true);
    foldSlider->setValue(0);
    foldSlider->setVisible(false);
    foldLabel->setVisible(false);
    foldCheckBox->setChecked(false);
    foldCheckBox->setVisible(false);

    callText->setVisible(false);
    callSlider->setEnabled(true);
    callSlider->setValue(0);
    callSlider->setVisible(false);
    callLabel->setVisible(false);
    callCheckBox->setChecked(false);
    callCheckBox->setVisible(false);

    raiseText->setVisible(false);
    raiseSlider->setEnabled(true);
    raiseSlider->setValue(0);
    raiseSlider->setVisible(false);
    raiseLabel->setVisible(false);
    raiseCheckBox->setChecked(false);
    raiseCheckBox->setVisible(false);

    checkText->setVisible(false);
    checkSlider->setEnabled(true);
    checkSlider->setValue(0);
    checkSlider->setVisible(false);
    checkCheckBox->setChecked(false);
    checkCheckBox->setVisible(false);
    checkLabel->setVisible(false);

    betText->setVisible(false);
    betSlider->setEnabled(true);
    betSlider->setValue(0);
    betSlider->setVisible(false);
    betLabel->setVisible(false);
    betCheckBox->setChecked(false);
    betCheckBox->setVisible(false);
}

void StrategyExplorer::toggleFoldLock(int state) {
    if (getCurrentNode()) {
        // Actualizar el estado de bloqueo de Fold en el nodo
        this->qSolverJob->get_solver()->getGameTree().get()->getActionLocked()[ActionType::Fold] = (state == Qt::Checked);

        if (state == Qt::Checked) {
            // Se activa el CheckBox
            this->addActionLocked(GameActions(GameTreeNode::PokerActions::FOLD,-1)); // Añadir la acción FOLD al vector actionsLocked
        } else {
            // Se desactiva el CheckBox
            this->removeActionLocked(GameActions(GameTreeNode::PokerActions::FOLD,-1));
        }
        this->updateLockedSliders();
    }
}

void StrategyExplorer::toggleCallLock(int state) {
    if (getCurrentNode()) {
        // Actualizar el estado de bloqueo de Fold en el nodo
        this->qSolverJob->get_solver()->getGameTree().get()->getActionLocked()[ActionType::Call] = (state == Qt::Checked);

        if (state == Qt::Checked) {
            // Se activa el CheckBox
            this->addActionLocked(GameActions(GameTreeNode::PokerActions::CALL,-1)); // Añadir la acción FOLD al vector actionsLocked
        } else {
            // Se desactiva el CheckBox
            this->removeActionLocked(GameActions(GameTreeNode::PokerActions::CALL,-1));
            callSlider->setEnabled(false);
        }
        this->updateLockedSliders();
    }
}

void StrategyExplorer::toggleRaiseLock(int state) {
    if (getCurrentNode()) {
        // Actualizar el estado de bloqueo de Fold en el nodo
        this->qSolverJob->get_solver()->getGameTree().get()->getActionLocked()[ActionType::Raise] = (state == Qt::Checked);

        if (state == Qt::Checked) {
            // Se activa el CheckBox
            this->addActionLocked(GameActions(GameTreeNode::PokerActions::RAISE, 0)); // Añadir la acción FOLD al vector actionsLocked
        } else {
            // Se desactiva el CheckBox
            this->removeActionLocked(GameActions(GameTreeNode::PokerActions::RAISE, 0));
        }
        this->updateLockedSliders();
    }
}

void StrategyExplorer::toggleCheckLock(int state) {
    if (getCurrentNode()) {
        // Actualizar el estado de bloqueo de Fold en el nodo
        this->qSolverJob->get_solver()->getGameTree().get()->getActionLocked()[ActionType::Check] = (state == Qt::Checked);

        if (state == Qt::Checked) {
            // Se activa el CheckBox
            this->addActionLocked(GameActions(GameTreeNode::PokerActions::CHECK,-1)); // Añadir la acción FOLD al vector actionsLocked
        } else {
            // Se desactiva el CheckBox
            this->removeActionLocked(GameActions(GameTreeNode::PokerActions::CHECK,-1));
        }
        this->updateLockedSliders();
    }
}

void StrategyExplorer::toggleBetLock(int state) {
    if (getCurrentNode()) {
        // Actualizar el estado de bloqueo de Fold en el nodo
        this->qSolverJob->get_solver()->getGameTree().get()->getActionLocked()[ActionType::Bet] = (state == Qt::Checked);

        if (state == Qt::Checked) {
            // Se activa el CheckBox
            this->addActionLocked(GameActions(GameTreeNode::PokerActions::BET, 0)); // Añadir la acción FOLD al vector actionsLocked
        } else {
            // Se desactiva el CheckBox
            this->removeActionLocked(GameActions(GameTreeNode::PokerActions::BET, 0));
        }
        this->updateLockedSliders();
    }
}

void StrategyExplorer::updateLockedSliders() {
    if (this->getActionsLocked().size() >= this->getNodeActions().size() - 1) {
        foldSlider->setEnabled(false);
        callSlider->setEnabled(false);
        raiseSlider->setEnabled(false);
        betSlider->setEnabled(false);
        checkSlider->setEnabled(false);
    } else {
        if (!checkCheckBox->isChecked()) {
            checkSlider->setEnabled(true);
        }
        if (!foldCheckBox->isChecked()) {
            foldSlider->setEnabled(true);
        }
        if (!callCheckBox->isChecked()) {
            callSlider->setEnabled(true);
        }
        if (!raiseCheckBox->isChecked()) {
            raiseSlider->setEnabled(true);
        }
        if (!betCheckBox->isChecked()) {
            betSlider->setEnabled(true);
        }

        for (const GameActions& action : this->getActionsLocked()) {
            GameActions actionCopy = action;
            switch (actionCopy.getAction()) {
                case GameTreeNode::PokerActions::FOLD:
                    foldSlider->setEnabled(!foldCheckBox->isChecked());
                    break;
                case GameTreeNode::PokerActions::CALL:
                    callSlider->setEnabled(!callCheckBox->isChecked());
                    break;
                case GameTreeNode::PokerActions::RAISE:
                    raiseSlider->setEnabled(!raiseCheckBox->isChecked());
                    break;
                case GameTreeNode::PokerActions::BET:
                    betSlider->setEnabled(!betCheckBox->isChecked());
                    break;
                case GameTreeNode::PokerActions::CHECK:
                    checkSlider->setEnabled(!checkCheckBox->isChecked());
                    break;
                default: break;
            }
        }
    }
}

void StrategyExplorer::addActionLocked(GameActions action) {
    std::vector<GameActions> temp = this->getActionsLocked();
    temp.push_back(action);
    this->setActionsLocked(temp);
}

void StrategyExplorer::removeActionLocked(GameActions action) {
    std::vector<GameActions> temp = this->getActionsLocked();
    // Utilizar std::find_if con una función lambda para buscar el elemento
    auto it = std::find_if(temp.begin(), temp.end(),
                           [&action](const GameActions& a) {
                               GameActions actionCopy = action; // Crear una copia del objeto GameActions
                               GameActions aCopy = a; // Crear una copia del objeto GameActions
                               return aCopy.getAction() == actionCopy.getAction() && aCopy.getAmount() == actionCopy.getAmount();
                           });

    // Si se encuentra el elemento, eliminarlo del vector
    if (it != temp.end()) {
        temp.erase(it);
        this->setActionsLocked(temp);
    }
}

ActionNode* StrategyExplorer::getCurrentNode() {
    // Obtener el nodo seleccionado del TableStrategyModel
    TreeItem* treeItem = tableStrategyModel->treeItem;

    // Castear el nodo a ActionNode
    ActionNode* currentNode = dynamic_cast<ActionNode*>(treeItem->m_treedata.lock().get());

    // Verificar si el casting fue exitoso
    if (currentNode) {
        return currentNode;
    } else {
        // Manejar el caso en que el nodo no sea un ActionNode
        return nullptr;
    }
}

void StrategyExplorer::item_expanded(const QModelIndex& index){
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    int num_child = item->childCount();
    for (int i = 0;i < num_child;i ++){
        TreeItem* one_child = item->child(i);
        if(one_child->childCount() != 0)continue;
        this->ui->gameTreeView->tree_model->reGenerateTreeItem(one_child->m_treedata.lock()->getRound(),one_child);
    }
}

void StrategyExplorer::process_board(TreeItem* treeitem){
    vector<string> board_str_arr = string_split(this->qSolverJob->board,',');
    vector<Card> cards;
    for(string one_board_str:board_str_arr){
        cards.push_back(Card(one_board_str));
    }
    if(treeitem != NULL){
        if(treeitem->m_treedata.lock()->getRound() == GameTreeNode::GameRound::TURN && !this->tableStrategyModel->getTrunCard().empty()){
            cards.push_back(Card(this->tableStrategyModel->getTrunCard()));
        }
        else if(treeitem->m_treedata.lock()->getRound() == GameTreeNode::GameRound::RIVER){
            if(!this->tableStrategyModel->getTrunCard().empty())
                cards.push_back(Card(this->tableStrategyModel->getTrunCard()));
            if(!this->tableStrategyModel->getRiverCard().empty())
                cards.push_back(Card(this->tableStrategyModel->getRiverCard()));
        }
    }
    this->ui->boardLabel->setText(QString("<b>%1: </b>").arg(tr("board")) + Card::boardCards2html(cards));
}

void StrategyExplorer::process_treeclick(TreeItem* treeitem){
    shared_ptr<GameTreeNode> treenode = treeitem->m_treedata.lock();
    if(treenode->getType() == GameTreeNode::GameTreeNodeType::ACTION){
        shared_ptr<ActionNode> actionnode = static_pointer_cast<ActionNode>(treenode);
        QString action_str = QString("<b>%1 %2</b>").arg(actionnode->getPlayer() == 0?tr("IP"):tr("OOP"),tr(" decision node"));
        this->ui->nodeDisplayLabel->setText(action_str);
    }
    else if(treenode->getType() == GameTreeNode::GameTreeNodeType::CHANCE){
        QString chance_str = tr("<b>Chance node</b>");
        this->ui->nodeDisplayLabel->setText(chance_str);
    }
    else if(treenode->getType() == GameTreeNode::GameTreeNodeType::TERMINAL){
        QString terminal_str = tr("<b>Terminal node</b>");
        this->ui->nodeDisplayLabel->setText(terminal_str);
    }
    else if(treenode->getType() == GameTreeNode::GameTreeNodeType::SHOWDOWN){
        QString showdown_str = tr("<b>Showdown node</b>");
        this->ui->nodeDisplayLabel->setText(showdown_str);
    }
}

void StrategyExplorer::item_clicked(const QModelIndex& index){
    try{
        this->resetOptionPercentages();
        this->clearActionsLocked();
        TreeItem * treeNode = static_cast<TreeItem*>(index.internalPointer());
        this->process_treeclick(treeNode);
        this->process_board(treeNode);
        this->tableStrategyModel->setGameTreeNode(treeNode);
        this->tableStrategyModel->updateStrategyData();
        this->ui->strategyTableView->viewport()->update();
        this->roughStrategyViewerModel->onchanged();
        this->ui->roughStrategyView->triger_resize();
        this->ui->roughStrategyView->viewport()->update();
        this->updateNodeOptions();
    }
    catch (const runtime_error& error)
    {
        qDebug().noquote() << tr("Encountering error:");//.toStdString() << endl;
        qDebug().noquote() << error.what() << "\n";
    }
}

void StrategyExplorer::updateNodeOptions() {
    // Obtener el nodo actual del árbol
    ActionNode* currentNode = getCurrentNode();

    if (currentNode) {
        std::vector<GameActions> actionsTemp;
        // Mostrar u ocultar los controles según las acciones posibles
        bool canFold = false, canCall = false, canRaise = false, canBet = false, canCheck = false;
        for (const GameActions& action : currentNode->getActions()) {
            GameActions actionCopy = action;
            //GameActions newAction;
            switch (actionCopy.getAction()) {
                case GameTreeNode::PokerActions::FOLD:
                    //newAction = GameActions(GameTreeNode::PokerActions::FOLD, -1);
                    canFold = true;
                    break;
                case GameTreeNode::PokerActions::CALL:
                    //newAction = GameActions(GameTreeNode::PokerActions::CALL, -1);
                    canCall = true;
                    break;
                case GameTreeNode::PokerActions::RAISE:
                    //newAction = GameActions(GameTreeNode::PokerActions::RAISE, 0);
                    canRaise = true;
                    break;
                case GameTreeNode::PokerActions::BET:
                    //newAction = GameActions(GameTreeNode::PokerActions::BET, 0);
                    canBet = true;
                    break;
                case GameTreeNode::PokerActions::CHECK:
                    //newAction = GameActions(GameTreeNode::PokerActions::CHECK, -1);
                    canCheck = true;
                    break;
                default: break;
            }
            auto it = std::find_if(actionsTemp.begin(), actionsTemp.end(),
            [&actionCopy](const GameActions& a) {
               GameActions actionCopyCopy = actionCopy; // Crear una copia del objeto GameActions
               GameActions aCopy = a;
               return aCopy.getAction() == actionCopyCopy.getAction();
            });
            if (it == actionsTemp.end()) {
                // No existe, añadirlo
                actionsTemp.push_back(actionCopy);
            }
        }

        this->setNodeActions(actionsTemp);

        foldText->setVisible(canFold);
        foldSlider->setVisible(canFold);
        foldSlider->setEnabled(canFold);
        foldSlider->setValue(0);
        foldLabel->setVisible(canFold);
        foldCheckBox->setVisible(canFold);

        callText->setVisible(canCall);
        callSlider->setVisible(canCall);
        callSlider->setEnabled(canCall);
        callSlider->setValue(0);
        callLabel->setVisible(canCall);
        callCheckBox->setVisible(canCall);

        raiseText->setVisible(canRaise);
        raiseSlider->setVisible(canRaise);
        raiseSlider->setEnabled(canRaise);
        raiseSlider->setValue(0);
        raiseLabel->setVisible(canRaise);
        raiseCheckBox->setVisible(canRaise);

        checkText->setVisible(canCheck);
        checkSlider->setVisible(canCheck);
        checkSlider->setEnabled(canCheck);
        checkSlider->setValue(0);
        checkLabel->setVisible(canCheck);
        checkCheckBox->setVisible(canCheck);

        betText->setVisible(canBet);
        betSlider->setVisible(canBet);
        betSlider->setEnabled(canBet);
        betSlider->setValue(0);
        betLabel->setVisible(canBet);
        betCheckBox->setVisible(canBet);

        float raisePercentage = 0.0f;
        float betPercentage = 0.0f;

        for(pair<GameActions,pair<float,float>> strategy : roughStrategyViewerModel->tableStrategyModel->total_strategy)
        {
            QString options;
            GameActions one_action = strategy.first;
            float one_strategy_float = strategy.second.second * 100;
            if(one_action.getAction() ==  GameTreeNode::PokerActions::FOLD){
                foldLabel->setText(QString::number(one_strategy_float,'f',1));
                foldSlider->setValue(round(one_strategy_float));
            }
            else if(one_action.getAction() ==  GameTreeNode::PokerActions::CALL){
                callLabel->setText(QString::number(one_strategy_float,'f',1));
                callSlider->setValue(round(one_strategy_float));
            }
            else if(one_action.getAction() ==  GameTreeNode::PokerActions::CHECK){
                checkLabel->setText(QString::number(one_strategy_float,'f',1));
                checkSlider->setValue(round(one_strategy_float));
            }
            else if(one_action.getAction() ==  GameTreeNode::PokerActions::BET){
                betPercentage += one_strategy_float;
                betLabel->setText(QString::number(betPercentage,'f',1));
                betSlider->setValue(round(betPercentage));
            }
            else if(one_action.getAction() ==  GameTreeNode::PokerActions::RAISE){
                raisePercentage += one_strategy_float;
                raiseLabel->setText(QString::number(raisePercentage,'f',1));
                raiseSlider->setValue(round(raisePercentage));
            }
        }
    }
}

void StrategyExplorer::selection_changed(const QItemSelection &selected,
                                         const QItemSelection &deselected){
}

void StrategyExplorer::on_turnCardBox_currentIndexChanged(int index)
{
    if(this->cards.size() > 0 && index < this->cards.size()){
        this->tableStrategyModel->setTrunCard(this->cards[index]);
        this->tableStrategyModel->updateStrategyData();
        // TODO this somehow cause bugs, crashes, why?
        //this->roughStrategyViewerModel->onchanged();
        //this->ui->roughStrategyView->viewport()->update();
        this->process_board(this->tableStrategyModel->treeItem);
    }
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
}

void StrategyExplorer::on_riverCardBox_currentIndexChanged(int index)
{
    if(this->cards.size() > 0  && index < this->cards.size()){
        this->tableStrategyModel->setRiverCard(this->cards[index]);
        this->tableStrategyModel->updateStrategyData();
        //this->roughStrategyViewerModel->onchanged();
        //this->ui->roughStrategyView->viewport()->update();
        this->process_board(this->tableStrategyModel->treeItem);
    }
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
}

void StrategyExplorer::update_second(){
    if(this->cards.size() > 0){
        this->tableStrategyModel->updateStrategyData();
        this->roughStrategyViewerModel->onchanged();
        this->ui->roughStrategyView->viewport()->update();
        this->process_board(this->tableStrategyModel->treeItem);
    }
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
}


void StrategyExplorer::onMouseMoveEvent(int i,int j){
    this->detailWindowSetting.grid_i = i;
    this->detailWindowSetting.grid_j = j;
    this->ui->detailView->viewport()->update();
}

void StrategyExplorer::on_strategyModeButtom_clicked()
{
    this->detailWindowSetting.mode = DetailWindowSetting::DetailWindowMode::STRATEGY;
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
    this->roughStrategyViewerModel->onchanged();
    this->ui->roughStrategyView->viewport()->update();
}

void StrategyExplorer::on_ipRangeButtom_clicked()
{
    this->detailWindowSetting.mode = DetailWindowSetting::DetailWindowMode::RANGE_IP;
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
    this->roughStrategyViewerModel->onchanged();
    this->ui->roughStrategyView->viewport()->update();
}

void StrategyExplorer::on_oopRangeButtom_clicked()
{
    this->detailWindowSetting.mode = DetailWindowSetting::DetailWindowMode::RANGE_OOP;
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
    this->roughStrategyViewerModel->onchanged();
    this->ui->roughStrategyView->viewport()->update();
}

void StrategyExplorer::on_evModeButtom_clicked()
{
    this->detailWindowSetting.mode = DetailWindowSetting::DetailWindowMode::EV;
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
    this->ui->roughStrategyView->viewport()->update();
}

void StrategyExplorer::on_evOnlyModeButtom_clicked()
{
    this->detailWindowSetting.mode = DetailWindowSetting::DetailWindowMode::EV_ONLY;
    this->ui->strategyTableView->viewport()->update();
    this->ui->detailView->viewport()->update();
    this->roughStrategyViewerModel->onchanged();
    this->ui->roughStrategyView->viewport()->update();
}
