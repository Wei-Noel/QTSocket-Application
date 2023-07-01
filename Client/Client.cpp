#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTcpSocket>
#include <QLineEdit>

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
	QPushButton* connectButton = new QPushButton("Connect to Server", &window);
	buttonLayout->addWidget(connectButton);
	QPushButton* sendButton = new QPushButton("Send", &window);
	sendButton->setEnabled(false);
	buttonLayout->addWidget(sendButton);
	layout.addLayout(buttonLayout);

	//创建一个QTcpSocket对象，用于和服务器通信
	QTcpSocket clientSocket;
	//当点击connectButton时，判断客户端是否已经连接到服务器，如果是，则断开连接，如果不是，则连接到服务器
	QObject::connect(connectButton, &QPushButton::clicked, [&]() {
		if (!clientSocket.isOpen()) {
			// Connect to the server
			clientSocket.connectToHost("localhost", 8888);
		}
		else
		{
			// Disconnect from the server
			clientSocket.disconnectFromHost();
			clientSocket.close();
		}
		});
	//当客户端成功连接到服务器时，会触发connected信号
	QObject::connect(&clientSocket, &QTcpSocket::connected, [&]() {
		output->appendPlainText("Connected to server");
		connectButton->setText("DisConnect");
		sendButton->setEnabled(true);
		});
	//当客户端和服务器断开连接时，会触发disconnected信号
	QObject::connect(&clientSocket, &QTcpSocket::disconnected, [&]() {
		output->appendPlainText("Disconnected from server");
		connectButton->setText("Connect");
		sendButton->setEnabled(false);
		clientSocket.close(); // Close the socket
		});

	//当客户端接收到服务器发送的数据时，会触发readyRead信号
	QObject::connect(&clientSocket, &QTcpSocket::readyRead, [&]() {
		while (clientSocket.canReadLine()) {
			QByteArray data = clientSocket.readLine();
			QString message = QString(data).trimmed();
			output->appendPlainText("Server: " + message);
		}
		});

	//当点击sendButton时，判断客户端是否已经连接到服务器，如果是，则发送数据到服务器
	QObject::connect(sendButton, &QPushButton::clicked, [&]() {
		QString message = input->text();
		if (!message.isEmpty()) {
			QByteArray data = message.toUtf8() + '\n';
			output->appendPlainText("Client: " + message);
			clientSocket.write(data);
			input->clear();
		}
		});

	window.show();

	return app.exec();
}
