#ifndef NTB_PDLLIST_H
#define NTB_PDLLIST_H
#include "ntb_spinlock.h"
namespace ntb
{
template <typename T>
class List
{
public:
	uint size=0;
	struct node
	{
		node *prev=nullptr;
		node *next=nullptr;
		T data;
	};
	node *end;
	node *beg;


	List(uint length)
	{
		beg=nullptr;
		end=nullptr;
		free_list = new node[length];

		for (uint i=0; i<length-1; i++)
		{
			free_list[i].next=&free_list[i+1];
		}
		free_list[length-1].next=nullptr;
	}
	~List()noexcept
	{
		delete[]free_list;
	}
	bool nodeAdd(T& obj)noexcept
	{
		lock.lock();
		if (free_list == nullptr)
		{
			lock.unlock();
			return false;
		}
		if(beg!=nullptr)//not empty?
		{
			end->next=free_list;
			free_list->prev=end;
		}
		end=free_list;
		end->data=std::move(obj);
		if(beg==nullptr)//empty?
			beg=end;
		free_list = free_list->next;
		++size;
		lock.unlock();
		return true;
	}
	bool nodeAdd2(T& obj,int evfd)noexcept
	{
		lock.lock();
		if (free_list == nullptr)
		{
			lock.unlock();
			return false;
		}
		if(beg!=nullptr)//not empty?
		{
			end->next=free_list;
			free_list->prev=end;
		}
		end=free_list;
		end->data=std::move(obj);
		if(beg==nullptr)//empty?
		{
			if(-1==eventfd_write(evfd,0))//cancel added node
			{
				end->next=free_list;
				free_list=end;
				end=nullptr;
				lock.unlock();
				return false;
			}
			beg=end;
		}
		free_list = free_list->next;
		++size;
		lock.unlock();
		return true;

	}
	node* nodeAlloc()
	{
		node * ret;
		lock.lock();
		if (free_list != nullptr)
		{
			ret=free_list;
			free_list = free_list->next;
			ret->prev=ret;// remove nullptr to be able to unlink allocated but not linked
			lock.unlock();
			return  ret;
		}
		lock.unlock();
		return nullptr;

	}
	void nodeLinkBack(node* n)
	{
		lock.lock();
		if(beg!=nullptr)//not empty?
		{
			end->next=n;
			n->prev=end;
		}
		end=n;
		if(beg==nullptr)//empty?
		{
			beg=end;
		}
		++size;
		lock.unlock();
	}
	bool nodeUnlink(node *n)noexcept
	{
		lock.lock();
		if(beg==n)
		{
			if(beg!=end)
			{
				beg=n->next;
				n->prev=nullptr;// for existance check
			}
			else
			{
				beg=nullptr;
				end=nullptr;
			}
		}
		else if(end==n)
		{
			end=n->prev;
			n->prev=nullptr;// for existance check
		}
		else
		{
			if(n->prev!=nullptr)//existance check
			{
				n->prev->next=n->next;
				n->next->prev=n->prev;
				n->prev=nullptr;// for existance check
			}
			else
			{
				lock.unlock();
				return false;
			}
		}
		n->next=free_list;
		free_list=n;
		--size;
		lock.unlock();
		return  true;
	}
	bool isFull()noexcept
	{
		lock.lock();
		if(free_list!=nullptr)
		{
			lock.unlock();
			return false;
		}
		lock.unlock();
		return true;
	}
	bool isEmpty()noexcept
	{
		lock.lock();
		if(beg!=nullptr)
		{
			lock.unlock();
			return false;
		}
		lock.unlock();
		return true;
	}
private:
	Spinlock lock;
	node *free_list;
};
}//namespace end

#endif // NTB_PDLLIST_H
