#pragma once
#include<queue>
#include<pthread.h>
using namespace std;
using callback = void (*)(void* arg);
//任务类
struct Task
{
	Task()
	{
		function = nullptr;
		arg = nullptr;
	}
	Task(callback f, void* arg)
	{
		this->arg = arg;
		this->function = f;
	}
	callback function;
	void* arg;
};

//任务队列类
class TaskQueue
{
private:
	queue<Task> m_taskQ;
	pthread_mutex_t m_mutex;
public:
	TaskQueue();
	~TaskQueue();
	//添加任务
	void addTask(Task task);
	void addTask(callback f,void *arg);
	//取出一个任务
	Task takeTask();
	//获取任务个数
	inline int taskNumber()
	{
		return m_taskQ.size();
	}
	
};

