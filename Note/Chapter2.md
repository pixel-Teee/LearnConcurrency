如何开启这些线程，如何检查它们已经完成，如何监视它们？



C++标准库通过std::thread来管理线程。



这章节是关于开启一个线程，等待它去完成，或者让它在后台执行。

传递额外的参数到线程函数，以及转换所有权到另一个线程对象。

最好，会有以及如何选择线程去使用，还有标识特定的线程。



# Basic thread management

每个C++程序都至少有一个线程，被C++运行时启动，这个线程运行main()。

当入口点函数返回的时候，线程退出。你将看到，如果你有个std::thread，你需要等待它去完成。



## Launching a thread



一些简单的情况，任务只是一个朴素的，普通的，void-return函数，没有输入参数。

这个函数在它自己的线程上运行，直到它返回。

另一个情况，则是，任务可以是一个函数对象，采取额外的参数，并且执行一些独立的操作，

通过一些消息系统描述，当它运行的时候，当线程停止的时候，会有信号发过来。



开始一个线程的方式：

```c++
void do_some_work();
std::thread my_thread(do_some_work);
```



std::thread可以与任何可调用对象类型一起工作，所以，你可以传递一个拥有调用操作符的类的实例，到std::thread。

```c++
class background_task
{
public:
	void operator()()const
	{
		do_something();
		do_something_else();
	}
};
background_task f;
std::thread my_thread(f);
```



这个情况下，被提供的函数对象被拷贝到**属于新的创建的线程的存储**里面，在这里进行执行和调用。

因此，这是必要的，就是拷贝行为要等价于原先的。



要小心这种情况：

```c++
std::thread my_thread(background_task());
```



声明了一个函数my_thread，输入一个单一的参数(一个类型指针指向background_task，或者一个background_task对象)，并且返回一个std::thread对象。而不是开启一个新线程。



可以有多种避免情况，比如传入一个具名对象，或者是下面的：

```C++
std::thread my_thread((background_task()));

std::thread my_thread{background_task()};
```



第一个例子，()圆括号阻止了解释为一个函数声明，因为允许my_thread被声明为类型std::thread的一个变量。

第二个例子，使用了新的uniform初始化语义，也会视为一个变量。



还有一种可调用对象，可以避免这种情况，叫做lambda表达式。

```c++
std::thread my_thread([]{
	do_something();
	do_something_else();
});
```



一旦开始一个线程，你需要显式决定是否去等待它完成(通过join)，或者留下它自己运行(通过detach)。

如果在std::thread对象会被销毁之前没有做出决定，那么你的程序就终止了(std::thread析构函数调用std::terminate())。

因此，你需要保证线程是正确的joined或者detached，即使出现意外情况。

你需要做出决定，在std::thread被销毁的时候，线程可能本身已经完成很久了，在你join它或者detach它的时候。

**如果你detach它，那么这个线程可能会持续运行好久，在std::thread对象被销毁的之后。**



如果你不等待线程去完成，你需要去保证被线程访问的数据是有效的，直到线程已经完成的时候。



另一个遇到的情况是，当线程函数持有指针或者引用到一个局部变量，并且线程没有完成，当函数退出的时候。



一个函数返回，当一个线程仍然访问局部变量的时候：

```c++
struct func
{
	int& i;
	func(int& i_):i(_i){}
	void operator()()
	{
		for(unsigned j = 0; j < 100000; ++j)
		{
			do_something(i);
		}
	}
};

void oops()
{
	int some_local_state = 0;
	func my_func(some_local_state);
	std::thread my_thread(my_func);
	my_thread.detach();
}
```



当oops退出的时候，my_thread将可能依然运行。因为调用了detach函数。

如果thread依然执行，那么下一次调用do_something(i)会访问一个销毁的变量。



一个常见的方法去处理这个情况，是使得thread函数自包含，并且拷贝数据到线程，

而不是共享数据。如果你使用一个可调用对象对于你的线程函数，这个对象本身被拷贝

到线程，那么原先的对象可以立即销毁。



另一种方式，你可以保证线程在函数退出之前完成，**通过joining这个thread。**



## Waiting for a thread to complete

换成join。



但是在这种情况下，在单独的线程上运行函数是没有意义的，以外第一个线程在此期间不会做任何有意义的事情，

但是，在真正的代码里面，原先的线程要么做它自己的工作，或者它开启其它的线程做一些有益的事情，

在等待它们完成之时。



join是简单和暴力的，要么你等待一个线程，要么你不用。

如果你需要更多细粒度的控制等待一个线程，比如检查一个线程是否完成，或者等待一段时间，

那么你必须使用可选的机制，比如**条件变量和future**。



调用join()也会清空任何存储的关系，那么std::thread对象不在与现在完成的线程相关联，也不在于任何线程相关联，意味着你可以调用join()一次，对于一个给定的线程，一旦你调用join()，std::thread不再是可连接的，

joinable()会返回false。



join会阻塞当前线程等待新线程，然后当新线程执行完毕后，**thread对象会和保存的线程解除关系。**



## Waiting in exceptional circumstances

在特殊情况下等待。



如果你选择调用join，那么你需要小心地选择一个放置这个join函数位置的地方。

意味着join的调用会受到影响而被跳过，**如果一个异常被抛出在一个线程已经被启动后，在join调用之前。**



为了避免你的应用被终止，当一个异常被抛出的时候，你因此需要去决定做什么在这种情况下。

通常，如果你企图调用join在一个非异常情况下，你也需要去调用join在一个异常出现的情况下，去避免生命周期问题。



```c++
struct func;

void f()
{
	int some_local_state = 0;
	func my_func(some_local_state);
	std::thread t(my_func);
	
	try
	{
		do_something_in_current_thread();
	}
	catch(...)
	{
		t.join();
		throw;
	}
	t.join();
}
```

这里do_something_in_current_thread如果抛出异常，应该让t.join()继续执行的，因此要catch异常。

**然后正常的执行流程那里也要join一下。**



try/catch块保证了一个线程访问一个local state在一个函数退出之前完成，

是否一个函数退出正常或者一个异常。



try/catch块是冗长的，并且容易导致作用域变得错误，所以这不是一个理想情况。



一种方式是使用standard Resource Acquistion Is Initialization(RAII)资源获取即初始化，提供一个类，join()在它的构造函数里面。

```c++
class thread_guard
{
	std::thread& t;
public:
	explicit thread_guard(std::thread& t_):
	t(_t)
	{}
	~thread_guard()
	{
		if(t.joinable())
		{
			t.join();
		}
	}
	thread_guard(thread_guard const&) = delete;
	thread_guard& operator=(thread_guard const&) = delete;
};

struc func;

void f()
{
	int some_local_state = 0;
	func my_func(some_local_state);
	std::thread t(my_func);
	thread_guard g(t);
	
	do_something_in_current_thread();
}
```



当现在的线程到到达f函数的末尾的时候，**局部变量以构造的相反顺序销毁。**

作为结果，thread_guard对象g被第一个销毁，然后thread被join在析构函数里面。

**这个甚至发生，如果函数退出，由于do_something_in_current_thread抛出一个异常。**



析构函数里面首先调用joinable，检查一下thread之前是不是已经被join过了。

这个很重要，因为一个线程只能被join一次。



如果使用的是detach，则不需要考虑这种异常安全的问题了。



## Running threads in the background

调用detach在一个std::thread对象上，会留下这个线程运行在背后，没有直接沟通的方式。

再也不可能去等待这个线程去完成，如果一个线程变得detached，它不可能获取它引用的一个std::thread对象，

因此它再也不能被join了。Detached的线程运行在真正的后台，**所有权和控制被传递到C++ Runtime库，**

保证了被关联到thread的资源被正确的回收，当线程退出的时候。



Detached的线程也经常被叫做**daemon threads(守护线程)**在UNIX的一个守护进程的概念之后，运行在后台，

没有任何显式地用户接口。

这样的线程是典型的持续时间长的，它们会运行在一个应用的整个生命周期。

执行一个后台任务，比如监视文件系统，从对象缓存中清空未使用的条目，或者优化数据结构。



这是有意义的，去使用一个detached线程，这里有一种机制来识别线程何时完成，或者线程用来使用**触发并忘记**的任务。



当调用detach的时候，在所有调用完毕之后，std::thread对象不在关联到真正的线程的执行，因为也就不再可以joinable了。

```c++
std::thread t(do_background_work);
t.detach();
assert(!t.joinable());
```



只有joinable返回true的时候，才能调用detach和join。



比如字词处理程序，有很多窗口，但是每个可以跑到不同的线程上。

```c++
//Detaching a thread to handle other documents

void edit_document(std::string const& filename)
{
	open_document_and_display_gui(filename);
	while(!done_editing())
	{
		user_command cmd = get_user_input();
		if(cmd.type == open_new_document)
		{
			std::string const new_name = get_filename_from_user();
			std::thread t(edit_document, new_name);
			t.detach();
		}
		else
		{
			process_user_input(cmd);
		}
	}
}
```

当用户选择去打开一个新文档的时候，你促使它们打开文档，开启一个新的线程去打开文档，然后detach它们，

然后文件名作为参数串进去。



# Passing aruguments to a thread function



//23





































