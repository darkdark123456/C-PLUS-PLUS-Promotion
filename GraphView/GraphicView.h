#pragma once
#include  <qgraphicsscene.h>
#include  <qgraphicsview.h>
#include  <qpixmap.h>
#include  <qfile.h>
#include  <qfiledialog.h>
#include  <qevent.h>
#include  <qpen.h>
#include  <qpainterpath.h>
#include  <qlineedit.h>
#include  <qgraphicsitem.h>
#include  <qinputdialog.h>
#include  <qmessagebox.h>
#include  <qtextedit.h>
#include  <qpushbutton.h>
#include  <qlist.h>
#include  <qpolygon.h>



class GraphicView : public QGraphicsView
{
	public:
		explicit GraphicView() {
			setTransformationAnchor(QGraphicsView::AnchorUnderMouse); // 设置图像变换时基于鼠标
			setResizeAnchor(QGraphicsView::AnchorUnderMouse); // 设置图片缩放大小时基于鼠标位置
			setDragMode(QGraphicsView::ScrollHandDrag);// 设置滚动拖拽
			pen.setColor(Qt::red);
			pen.setWidth(2);

			scene    = new QGraphicsScene(this);
			TextItem = new QGraphicsTextItem();
			pathItem = new QGraphicsPathItem();
			painter  = new QPainter();
			arr      = new QGraphicsPolygonItem();
			painter->setPen(pen);


			QFile file = QFileDialog::getOpenFileName(this, "Openfile", "", "*.jpg");
			QPixmap pixmap(file.fileName());
			scene->addPixmap(pixmap);
			this->setScene(scene);

			clearBtn = new QPushButton(this);
			clearBtn->setText("Clear Painter Path");

			arrowBtn = new QPushButton(this);
			arrowBtn->setText("Add an Arrow");

			arrowBtn->setGeometry(QRect(clearBtn->geometry().x(), clearBtn->geometry().y() + clearBtn->height(),
										clearBtn->width(), clearBtn->height()));

			// 使用lambda写法
			connect(clearBtn, &QPushButton::clicked, this, [&]()->void {
				if (!list.isEmpty()) {
					for (auto& item : list) {
						scene->removeItem(item);
					}
					list.erase(list.begin(), list.end());
				}
				});
			
			//使用继承的写法
			QObject::connect(arrowBtn, &QPushButton::clicked,this,&GraphicView::Function);
		};

		~GraphicView() {
			delete scene;
			delete TextItem;
			delete pathItem;
			delete clearBtn;
			delete arrowBtn;
		};

	public:
		void wheelEvent(QWheelEvent* event) override {
			int mousePostion= event->angleDelta().y(); // 判断前滚轮还是后滚轮
			if(mousePostion>0){
				scale(1.2, 1.2); // 放大

			}
			else{
				scale(0.5, 0.5); // 缩小
			}
		}

		void mouseDoubleClickEvent(QMouseEvent* event) {
			if (appendArrow) {
				appendArrow = 0;
			}
			scene->removeItem(this->TextItem);	
		}

		void mousePressEvent(QMouseEvent* event) override{
			// 左键画图 右键注释
			if (event->button() == Qt::LeftButton) {
				
				if (appendArrow) {
					arrowStart = mapToScene(event->pos());
					isDrawing = 0;
					isItem = 0;
					arrow << QPointF(50, 0) << QPointF(10, 10) << QPointF(5, 5);
					QGraphicsPolygonItem* arr = new QGraphicsPolygonItem(arrow);
					arr->setPos(arrowStart.x(), arrowStart.y());
					this->arr = arr;
					scene->addItem(this->arr);
					appendArrow = 0;
					return;
				}


				// 如果选中了一个item 进行的是item的移动操作
				QGraphicsItem* item = itemAt(event->pos());
				if (item && item->type() == QGraphicsTextItem::Type) {
					TextItem = dynamic_cast<QGraphicsTextItem*>(item);
					isItem = 1;
					isDrawing = 0;
					appendArrow = 0;
				}

				else if (item && item->type() == QGraphicsPolygonItem::Type) {
					arr = dynamic_cast<QGraphicsPolygonItem*>(item);
					isItem = 0;
					isDrawing = 0;
					appendArrow = 1;
				}


				// 如果没有选中item 进行的是画图操作
				else
				{
					isDrawing = 1;
					isItem = 0;
					appendArrow = 0;
					path.moveTo(mapToScene(event->pos()));
				}
			}

			if (event->button() == Qt::RightButton) {
				QString text = QInputDialog::getText(this, "Note", "");

				QGraphicsTextItem* TextItem = new QGraphicsTextItem();
				TextItem->setPlainText(text);
				TextItem->setDefaultTextColor(Qt::green);
				TextItem->setPos(mapToScene(event->pos()));
				TextItem->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
				
				scene->addItem(TextItem);

				if (TextItem->toPlainText() != "") {
					this->TextItem = TextItem;
				}
				
			}

		}

		void mouseMoveEvent(QMouseEvent* event) override {
			if (isDrawing) {
				path.lineTo(mapToScene(event->pos()));
				pathItem=scene->addPath(path, pen);
				list.push_back(pathItem);
			}

			if (isItem) {
				TextItem->setPos(mapToScene(event->pos()));
			}

			if (appendArrow) {
				arr->setPos(mapToScene(event->pos()));
			}

		}


		void mouseReleaseEvent(QMouseEvent* event) override {
			if (event->button() == Qt::LeftButton) {
				isDrawing = false;
				isItem = false;
				appendArrow = 0;
				path = QPainterPath();
			}
		}

	private slots:
		void Function() {
			appendArrow = 1;
		}

	private:
		QPainterPath path;
		QPainter* painter;
		QPen pen;
		bool isDrawing = 0;
		bool isItem = 0;
		bool appendArrow = 0;
		QGraphicsScene* scene;
		QGraphicsTextItem* TextItem;	
		QGraphicsPathItem* pathItem;
		QPoint lastapostion;
		QPushButton* clearBtn;
		QPushButton* arrowBtn;
		QList<QGraphicsPathItem*> list;
		QPointF arrowStart;
		QPolygonF arrow;
		QGraphicsPolygonItem* arr;
};

