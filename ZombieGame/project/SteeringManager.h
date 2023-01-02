#ifndef STEERINGMANAGER
#define STEERINGMANAGER
#include "Exam_HelperStructs.h"
#include "IExamInterface.h"
class SteeringManager final
{
public:
	SteeringManager(IExamInterface* pInterface, SteeringPlugin_Output* pSteering);
	~SteeringManager() = default;

	SteeringManager(const SteeringManager& inventoryManager) = delete;
	SteeringManager(SteeringManager&& inventoryManager) = delete;
	SteeringManager& operator=(const SteeringManager& inventoryManager) = delete;
	SteeringManager& operator=(SteeringManager&& inventoryManager) = delete;

	
	void AutoOrient(bool state);
	void Seek(const Elite::Vector2& target);
	void Flee(const Elite::Vector2& target, float factor = 2.f);
	void Wander();
	void SpinAround();
	void Face(const Elite::Vector2& target);
	void StopMovement();
private:
	IExamInterface* m_pInterface = nullptr;
	SteeringPlugin_Output* m_pSteering = nullptr;
};

#endif