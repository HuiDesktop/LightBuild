extern "C" {
	typedef struct Live2DManagedData_t {
		float x, y, scaleX, scaleY;
		void* model;
	} Live2DManagedData;

	typedef enum SetParameterType_t {
		SetParameterType_Set,
		SetParameterType_Add,
		SetParameterType_Multiply
	} SetParameterType;

	__declspec(dllexport) void l2dInit();

	/// <summary>
	/// ����Live2Dģ��
	/// </summary>
	/// <param name="dir">ģ���ļ���·������Ŀ¼�ָ�����β</param>
	/// <param name="file">.model3.json�ļ��������ð���Ŀ¼</param>
	/// <returns>ģ�����ݵ�ָ��</returns>
	__declspec(dllexport) Live2DManagedData* l2dLoadModel(const char* dir, const char* file);

	__declspec(dllexport) void l2dUpdateModelMatrix(Live2DManagedData* model);

	__declspec(dllexport) void l2dUpdate();

	__declspec(dllexport) void l2dPreUpdateModel(Live2DManagedData* model);

	__declspec(dllexport) void l2dUpdateModel(Live2DManagedData* model);

	__declspec(dllexport) int l2dHitTest(Live2DManagedData* data, const char* name, float x, float y);

	__declspec(dllexport) void l2dSetExpression(Live2DManagedData* data, const char* expid);

	__declspec(dllexport) void l2dSetMotion(Live2DManagedData* data, const char* group, int no, int priority);

	__declspec(dllexport) const void* l2dGetParameterId(const char* name);
	
	__declspec(dllexport) void l2dSetParameter(Live2DManagedData* data, const void* id, SetParameterType type, float value, float weight);
}
