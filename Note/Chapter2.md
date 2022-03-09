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



传递到std::thread构造函数的参数，需要牢记的是默认的情况下，参数是被拷贝到内部进行存储的，

它们可以被访问，通过新创建执行的线程。

即使在函数中相应的参数要求是一个引用。



```c++
void f(int i, std::string const& s);
std::thread t(f, 3, "hello");
```



这个创建了一个新的可执行线程关联到t，调用了f(3, "hello")。

即使f采用std::string作为第二个参数，字符串字面值被传递作为一个char const*，并且转换成一个std::string，

在新线程的语境中。这是尤其重要的，当一个被提供的参数是一个指针到一个自动变量的时候。

```C++
void f(int i, std::string const& s);

void oops(int some_param)
{
	char buffer[1024];
	sprintf(buffer, "%i", some_param);
	std::thread t(f, 3, buffer);
	t.detach();
}
```



指向局部变量buffer的指针被传递到新线程，这里有个极大的概率，函数oops将退出，在buffer转换成一个std::string在新线程内的时候，导致未定义的行为。

解决方法是cast到std::string，在传递buffer到std::thread构造函数的时候。

```C++
void f(int i, std::string const& s);

void not_oops(int some_param)
{
	char buffer[1024];
	sprintf(buffer, "%i", some_param);
	std::thread t(f, 3, std::string(buffer));
	t.detach();
}
```

这种情况，问题在于你是依赖于指针的隐式转换，buffer到std::string对象，要求作为一个函数参数，

因为std::thread构造函数拷贝了提供的值，**没有转换成要求的参数类型。**



原因在于，隐式转换发生在thread的构造函数内部，**我们提前转换，从而早早地就进行拷贝。**



这里有一种相反的情况，对象被拷贝了，但是你想要的是一个引用。

这个可能发生，如果线程更新的一个数据结构是通过引用传递的，例如：

```c++
void update_data_for_widget(widget_id w, widget_data& data);

void oops_again(widget_id w)
{
    widget_data data;
    std::thread t(update_data_for_widget, w, data);
    display_status();
    t.join();
    process_widget_data(data);//传递一个未更新的data
}
```

即使update_data_for_widget要求第二个参数通过引用传递，std::thread构造函数不知道这样，依然盲目地拷贝提供的值。当它调用update_for_widget，它将以传递一个引用到内部data的一份拷贝，而不是data的引用。



作为结果，当线程完成的时候，这些更新都会被扔弃，因为内部提供参数的内部拷贝被销毁了，并且

process_widget_data将会被传递一个未改变的data，而不是一个正确更新的版本。



如果你熟悉std::bind，这个解决方案会快捷地显而易见，你需要去包裹参数在std::ref里面。

```C++
std::thread t(update_data_for_widget, w, std::ref(data));
```



如果你熟悉std::bind，传递参数的语义将是不吃惊的，因为std::thread构造函数还有std::bind的操作被定义，依据同样的机制。比如，你可以传递一个成员函数，然后传递一个对象作为第一个参数。

```c++
class X
{
public:
	void do_lengthy_work();
};

X my_x;
std::thread t(&X::do_length_work, &my_x);
```



这个代码将会调用my_x.do_lengthy_work()在新线程上，因为my_x的地址被提供作为函数指针。

其余的参数会作为成员函数的参数。



另一个有趣的现象，对于提供的参数，是参数不能被拷贝，只能被移动。

被一个对象持有的数据被转移到另一个上，留下原先的对象为空。

另一个这样的类型是std::unique_ptr，提供自动的内存管理，对于动态分配的对象。

只有一个std::unique_ptr实例可以指向给定的对象在一个时间点，并且当这个实例被销毁的时候，指针指向的对象也会被删除。

**move构造和move赋值移动允许一个对象的所有权被转换到std::unique_ptr实例。**



这样一个转换会留下源对象一个NULL指针。

这个移动的值允许这个类型的对象作为一个函数的参数被接收或者从一个函数返回。

转换必须被直接地请求，通过调用std::move()。

下面的例子展示了std::move的用法，去转换一个动态对象的所有去到一个线程：

```c++
void process_big_object(std::unique_ptr<big_object>);

std::unqiue_ptr<big_object> p(new big_object);
p->prepare_data(42);
std::thread t(process_big_object, std::move(p));
```



通过描述std::move(p)在std::thread构造函数内，big_object的所有权第一次被转换到新创建的线程的内部，并且

然后到process_big_object。



标准库的一些类展现了和std::unqiue_ptr相同所有权语义的类，std::thread就是其中一个。



每个实例负责一个线程执行。这个所有权可以在实例中间转移，因为std::thread的实例是可以移动的，

即使它们不可以移动。



# Transferring ownership of a thread

这是std::thread发挥作用的地方。



这意味着一个运行的特定的线程的所有权可以在std::thread实例之间进行移动。

```c++
void some_function();
void some_other_function();
std::thread t1(some_function);
std::thread t2 = std::move(t1);
t1 = std::thread(some_other_function);
std::thread t3;
t3 = std::move(t2);
t1 = std::move(t3);
```



t1 = std::thread(some_other_function);

这里一个临时对象本身具备移动语义，不需要std::move显式地转移所有权。



t3是默认构造的，也就是它被创建没有任何关联运行的线程。



最后一个，t1已经有一个相关联的线程，std::terminate()被调用，去终止程序。

这个和std::thread的析构函数的一致性相同。

前面的节，你必须显式地等待一个线程**去完成或者detach**它在析构之前，

同样的操作在赋值上，你不能只是简单地去掉一个线程，通过赋值一个新值

到std::thread进行管理。



移动支持，意味着所有权可以轻松地转移出函数。

```C++
std::thread f()
{
	void some_function();
	return std::thread(some_function);//这里
}

std::thread g()
{
	void some_other_function(int);
	std::thread t(some_other_function, 42);
	return t;//这里
}
```



如果所有权应当被转移进一个函数，它可以只是接收一个std::thread的实例，

**通过传值作为一个参数：**

```c++
void f(std::thread t);

void g()
{
	void some_function();
	f(std::thread(some_function));
	std::thread t(some_function);
	f(std::move(t));
}
```



std::thread的移动支持的一个好处是你可以构建thread_guard，

让它真正地拥有thread的所有权。

这个避免了任何不高兴的结果，如果std::guard的生命周期比它引用的对象的生命周期

长的话，这也意味着没有人可以join或者detach线程，一旦所有权已经被转移到对象。

因为这个主要是为了保证线程是完整的，在作用域退出之前，**我把它称之为scoped_thread。**

```c++
class scoped_thread
{
	std::thread t;
public:
	explicit scoped_thread(std::thread t_):
	t(std::move(t_))
	{
		if(!t.joinable())
			throw std::logic_error("No thread");
	}
	~scoped_thread()
	{
		t.join();
	}
	scoped_thread(scoped_thread const&) = delete;
	scoped_thread& operator=(scoped_thread const&) = delete;
};

struct func;

void f()
{
	int some_local_state;
	scoped_thread t(std::thread(func(some_local_state)));
	
	do_something_in_current_thread();
}
```



std::thread的移动支持，允许std::thread对象的容器，

如果这些容器是移动敏感的。

```C++
void do_work(unsigned id);

void f()
{
	std::vector<std::thread> threads;
	
	for(unsigned i = 0; i < 20; ++i)
	{
		threads.push_back(std::thread(do_work, i));//spawn threads
	}
	std::for_each(threads.begin(), threads.end(),
	std::mem_fn(&std::thread::join));//Call join() on each thread in turn
}
```



如果线程被用了细分一个算法的工作，这是我们经常需要的，在返回到调用者的时候，所有的线程必须完成。

这个代码也说明了通过线程完成的工作是自给自足的，并且它们结果的操作纯粹是对共享数据的副作用。

如果f()返回一个值到调用者，依赖于这些线程的操作的执行结果的，然后，正如所写的，在线程终止后，

**必须通过检查共享数据来确定。**



可选的方案，对于传递操作的结果在线程之间将在第4章节讨论。



# Choosing the number of threads at runtime



C++标准库的一个特性，帮助这里，是std::thread::hardware_concurrency()。

这个函数返回一个线程的数量，可以真正地允许并发，对于一个程序的给定执行。



在一个多核系统上，它可能是CPU核心的数量。

这只是一个线索，函数可能返回0，如果这个信息是不可用的，但是它可以是一个有用的指导，在线程之间分离一个任务。

//29















































