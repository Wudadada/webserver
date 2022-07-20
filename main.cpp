#include "config.h"

int main(int argc, char* argv[]) {
	//�޸���Ҫ�����ݿ���Ϣ����¼�������룬����
	string user = "root";
	string passwd = "123";
	string databasename = "wqd";

	//�����н���
	Config config;
	config.parse_arg(argc, argv);

	Webserver server;

	//��ʼ��
	server.init(config.PORT, user, passwd, databasename, config.LOGWriter,
		config.OPT_LINGER, config.TRIGMode, config.sql_num, config, thread_num,
		config.close_log, config.actor_model);

	//��־
	server.log_writer();

	//���ݿ�
	server.sql_pool();

	//�̳߳�
	server.thread_pool();

	//����ģʽ
	server.trig_mode();

	//����
	server.eventLoop();

	return 0;
}