#include <QApplication>
#include <QWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTcpServer>
#include <QTcpSocket>
// 转发消息给除发送消息的客户端之外的所有客户端
void forwardMessage(const QByteArray& data, const QList<QTcpSocket*>& connections, QTcpSocket* sender) {
	for (auto connection : connections) {
		if (connection != sender) {
			connection->write(data);
		}
	}
}
// 创建一个QTcpServer对象，用于监听客户端的连接请求
QTcpServer server;
QList<QTcpSocket*> clientConnections;

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);

	QWidget window;
	QVBoxLayout layout(&window);

	QPlainTextEdit* output = new QPlainTextEdit(&window);
	output->setReadOnly(true);
	layout.addWidget(output);

	QLineEdit* input = new QLineEdit(&window);
	layout.addWidget(input);

	QHBoxLayout* buttonLayout = new QHBoxLayout;
	QPushButton* startButton = new QPushButton("Start Server", &window);
	buttonLayout->addWidget(startButton);
	QPushButton* sendButton = new QPushButton("Send", &window);
	buttonLayout->addWidget(sendButton);
	layout.addLayout(buttonLayout);

	// 当点击startButton时，判断服务器是否已经在监听，如果是，则停止监听，如果不是，则开始监听
	QObject::connect(startButton, &QPushButton::clicked, [&]() {
		if (server.isListening()) {
			//如果服务正在运行
			server.close();
			for (auto connection : clientConnections) {
				connection->close();
				connection->deleteLater();
			}
			clientConnections.clear();
			output->appendPlainText("Server stopped");
			startButton->setText("Start Server");
		}
		else {
			//服务没启动
			if (server.listen(QHostAddress::Any, 8888)) {
				output->appendPlainText("Server started. Listening on port 8888");
				startButton->setText("Stop Server");
			}
			else {
				output->appendPlainText("Failed to start server");
			}
		}
		});


	// 当有客户端连接到服务器时，会触发newConnection信号，我们在这里处理客户端的连接请求
	QObject::connect(&server, &QTcpServer::newConnection, [&]() {
		// 获取客户端的连接
		QTcpSocket* clientSocket = server.nextPendingConnection();
		// 将客户端的连接保存到clientConnections中
		clientConnections.append(clientSocket);
		output->appendPlainText("Client connected: " + clientSocket->peerAddress().toString());

		// 当客户端有数据发送过来时，会触发readyRead信号，我们在这里处理客户端发送过来的数据
		QObject::connect(clientSocket, &QTcpSocket::readyRead, [=]() {
			// 读取客户端发送过来的数据
			while (clientSocket->canReadLine()) {
				QByteArray data = clientSocket->readLine();
				QString message = QString(data).trimmed();// 去掉末尾的换行符
				output->appendPlainText("Client: " + message);
				QList <QTcpSocket*>::const_iterator list = clientConnections.begin();
				// 输出连接列表到 output
				forwardMessage(data, clientConnections, clientSocket);
			}
			});
		// 当客户端断开连接时，会触发disconnected信号，我们在这里处理客户端断开连接的事件
		QObject::connect(clientSocket, &QTcpSocket::disconnected, [&]() {
			if (clientConnections.contains(clientSocket)) {
				clientConnections.removeOne(clientSocket);
				clientSocket->deleteLater();
				output->appendPlainText("Client disconnected");
			}
			});
		});
	// 当点击sendButton时，将input中的文本发送给所有的客户端
	QObject::connect(sendButton, &QPushButton::clicked, [&]() {
		QString message = input->text();
		if (!message.isEmpty()) {
			QByteArray data = message.toUtf8() + '\n';
			output->appendPlainText("Server: " + message);
			for (auto connection : clientConnections) {
				connection->write(data);
			}
			input->clear();
		}
		});

	window.show();

	return app.exec();
}
