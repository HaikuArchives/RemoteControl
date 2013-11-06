template <class T>
class autodelete
{
	public:
		autodelete(T *obj):obj(obj),DoDel(true){}
		~autodelete(){if(DoDel)delete obj;}
		void DoDelete(bool Do){DoDel=Do;}
		
	private:
		T *obj;
		bool DoDel;
};
