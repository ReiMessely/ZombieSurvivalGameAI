#include "stdafx.h"
#include "Plugin.h"
#include "IExamInterface.h"

#include "HelperFuncts.h"
#include "Behaviors.h"
#include "AgentProps.h"

using namespace std;

//#define DEMO
#ifdef DEMO
#define DEMO_STEERING
#define DEMO_INPUT
#endif 

//Called only once, during initialization
void Plugin::Initialize(IBaseInterface* pInterface, PluginInfo& info)
{
	//Retrieving the interface
	//This interface gives you access to certain actions the AI_Framework can perform for you
	m_pInterface = static_cast<IExamInterface*>(pInterface);

	//Bit information about the plugin
	//Please fill this in!!
	info.BotName = "MinionExam";
	info.Student_FirstName = "Re�";
	info.Student_LastName = "Messely";
	info.Student_Class = "2DAE15";

	// -----------------------------------------------------------------------------------------

	m_pSteeringOutputData = new SteeringPlugin_Output();
	m_pAgentProps = new AgentProps();
	m_pAgentProps->enemyDetectionRadius = 10.f;
	m_pEntitiesInFOV = new std::vector<EntityInfo>();
	m_pHousesInFOV = new std::vector<HouseInfo>();

	// Initialise blackboard data
	Elite::Blackboard* pBlackboard = new Elite::Blackboard();
	pBlackboard->AddData("interface", static_cast<IExamInterface*>(m_pInterface));
	pBlackboard->AddData("steering", static_cast<SteeringPlugin_Output*>(m_pSteeringOutputData));
	pBlackboard->AddData("agentProps",static_cast<AgentProps*>(m_pAgentProps));
	pBlackboard->AddData("entitiesInFOV", static_cast<std::vector<EntityInfo>*>(m_pEntitiesInFOV));
	pBlackboard->AddData("housesInFOV", static_cast<std::vector<HouseInfo>*>(m_pHousesInFOV));

	// Root behavior is the first behavior that is connected to the root node.
	m_pBehaviorTree = new Elite::BehaviorTree(pBlackboard, new Elite::BehaviorSelector
	(
		{
			new Elite::BehaviorSequence
			(
				{
					new Elite::BehaviorConditional(&BT_Conditions::IsHouseInPOV),
					new Elite::BehaviorInvertConditional(&BT_Conditions::AgentInsideHouse),
					new Elite::BehaviorAction(&BT_Actions::GoToFirstHouse)
				}
			),
			new Elite::BehaviorAction(&BT_Actions::LootFOV),
			new Elite::BehaviorAction(&BT_Actions::SpinAround),
			new Elite::BehaviorAction(&BT_Actions::ChangeToWander)
		}
	));
}

//Called only once
void Plugin::DllInit()
{
	//Called when the plugin is loaded
}

//Called only once
void Plugin::DllShutdown()
{
	//Called when the plugin gets unloaded

	SAFE_DELETE(m_pBehaviorTree);
	SAFE_DELETE(m_pSteeringOutputData);
	SAFE_DELETE(m_pAgentProps);
	SAFE_DELETE(m_pEntitiesInFOV);
	SAFE_DELETE(m_pHousesInFOV);

	// No need to delete blackboard since behavior tree gains ownership
	//SAFE_DELETE(m_pBlackboard);
}

//Called only once, during initialization
void Plugin::InitGameDebugParams(GameDebugParams& params)
{
	params.AutoFollowCam = true; //Automatically follow the AI? (Default = true)
	params.RenderUI = true; //Render the IMGUI Panel? (Default = true)
	params.SpawnEnemies = true; //Do you want to spawn enemies? (Default = true)
	params.EnemyCount = 20; //How many enemies? (Default = 20)
	params.GodMode = false; //GodMode > You can't die, can be useful to inspect certain behaviors (Default = false)
	params.LevelFile = "GameLevel.gppl";
	params.AutoGrabClosestItem = true; //A call to Item_Grab(...) returns the closest item that can be grabbed. (EntityInfo argument is ignored)
	params.StartingDifficultyStage = 1;
	params.InfiniteStamina = false;
	params.SpawnDebugPistol = true;
	params.SpawnDebugShotgun = true;
	params.SpawnPurgeZonesOnMiddleClick = true;
	params.PrintDebugMessages = true;
	params.ShowDebugItemNames = true;
	params.Seed = 36; // 36 is demo seed
}

//Only Active in DEBUG Mode
//(=Use only for Debug Purposes)
void Plugin::Update(float dt)
{
	// DEBUG ONLY

#ifdef DEMO_INPUT
	//Demo Event Code
	//In the end your AI should be able to walk around without external input
	if (m_pInterface->Input_IsMouseButtonUp(Elite::InputMouseButton::eLeft))
	{
		//Update target based on input
		Elite::MouseData mouseData = m_pInterface->Input_GetMouseData(Elite::InputType::eMouseButton, Elite::InputMouseButton::eLeft);
		const Elite::Vector2 pos = Elite::Vector2(static_cast<float>(mouseData.X), static_cast<float>(mouseData.Y));
		m_Target = m_pInterface->Debug_ConvertScreenToWorld(pos);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Space))
	{
		m_CanRun = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Left))
	{
		m_AngSpeed -= Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Right))
	{
		m_AngSpeed += Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_G))
	{
		m_GrabItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_U))
	{
		m_UseItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_R))
	{
		m_RemoveItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyUp(Elite::eScancode_Space))
	{
		m_CanRun = false;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Delete))
	{
		m_pInterface->RequestShutdown();
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_KP_Minus))
	{
		if (m_InventorySlot > 0)
			--m_InventorySlot;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_KP_Plus))
	{
		if (m_InventorySlot < 4)
			++m_InventorySlot;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Q))
	{
		ItemInfo info = {};
		m_pInterface->Inventory_GetItem(m_InventorySlot, info);
		std::cout << (int)info.Type << std::endl;
	}
#endif
}

//Update
//This function calculates the new SteeringOutput, called once per frame
// F12 ON THIS "SteeringPlugin_Output" STRUCT TO VIEW FILE
SteeringPlugin_Output Plugin::UpdateSteering(float dt)
{
	if (!UpdateEntitiesInFOV())
	{
		std::cout << "Failed to update entities\n";
	}
	if (!UpdateHousesInFOV())
	{
		std::cout << "Failed to update houses\n";
	}

	SteeringPlugin_Output* pSteering{};
	
	m_pBehaviorTree->Update(dt);

	m_pBehaviorTree->GetBlackboard()->GetData("steering", pSteering);

#ifdef DEMO_STEERING
	//Use the Interface (IAssignmentInterface) to 'interface' with the AI_Framework
	auto agentInfo = m_pInterface->Agent_GetInfo();


	//Use the navmesh to calculate the next navmesh point
	//auto nextTargetPos = m_pInterface->NavMesh_GetClosestPathPoint(checkpointLocation);

	//OR, Use the mouse target
	auto nextTargetPos = m_pInterface->NavMesh_GetClosestPathPoint(m_Target); //Uncomment this to use mouse position as guidance

	auto vHousesInFOV = GetHousesInFOV();//uses m_pInterface->Fov_GetHouseByIndex(...)
	auto vEntitiesInFOV = GetEntitiesInFOV(); //uses m_pInterface->Fov_GetEntityByIndex(...)

	for (auto& e : vEntitiesInFOV)
	{
		if (e.Type == eEntityType::PURGEZONE)
		{
			PurgeZoneInfo zoneInfo;
			m_pInterface->PurgeZone_GetInfo(e, zoneInfo);
			//std::cout << "Purge Zone in FOV:" << e.Location.x << ", "<< e.Location.y << "---Radius: "<< zoneInfo.Radius << std::endl;
		}
	}

	//INVENTORY USAGE DEMO
	//********************

	if (m_GrabItem)
	{
		ItemInfo item;
		//Item_Grab > When DebugParams.AutoGrabClosestItem is TRUE, the Item_Grab function returns the closest item in range
		//Keep in mind that DebugParams are only used for debugging purposes, by default this flag is FALSE
		//Otherwise, use GetEntitiesInFOV() to retrieve a vector of all entities in the FOV (EntityInfo)
		//Item_Grab gives you the ItemInfo back, based on the passed EntityHash (retrieved by GetEntitiesInFOV)
		if (m_pInterface->Item_Grab({}, item))
		{
			//Once grabbed, you can add it to a specific inventory slot
			//Slot must be empty
			m_pInterface->Inventory_AddItem(m_InventorySlot, item);
		}
	}

	if (m_UseItem)
	{
		//Use an item (make sure there is an item at the given inventory slot)
		m_pInterface->Inventory_UseItem(m_InventorySlot);
	}

	if (m_RemoveItem)
	{
		//Remove an item from a inventory slot
		m_pInterface->Inventory_RemoveItem(m_InventorySlot);
	}

	//Simple Seek Behaviour (towards Target)
	pSteering->LinearVelocity = nextTargetPos - agentInfo.Position; //Desired Velocity
	pSteering->LinearVelocity.Normalize(); //Normalize Desired Velocity
	pSteering->LinearVelocity *= agentInfo.MaxLinearSpeed; //Rescale to Max Speed

	if (Distance(nextTargetPos, agentInfo.Position) < 2.f)
	{
		pSteering->LinearVelocity = Elite::ZeroVector2;
	}

	//steering.AngularVelocity = m_AngSpeed; //Rotate your character to inspect the world while walking
	pSteering->AutoOrient = true; //Setting AutoOrient to TRue overrides the AngularVelocity

	pSteering->RunMode = m_CanRun; //If RunMode is True > MaxLinSpd is increased for a limited time (till your stamina runs out)

	//SteeringPlugin_Output is works the exact same way a SteeringBehaviour output

//@End (Demo Purposes)
	m_GrabItem = false; //Reset State
	m_UseItem = false;
	m_RemoveItem = false;
#endif 

	return *pSteering;
}

//This function should only be used for rendering debug elements
void Plugin::Render(float dt) const
{
	//This Render function should only contain calls to Interface->Draw_... functions
	m_pInterface->Draw_SolidCircle(m_Target, .7f, { 0,0 }, { 1, 0, 0 });
}

bool Plugin::UpdateHousesInFOV()
{
	vector<HouseInfo>* vHousesInFOV{nullptr};
	if (m_pBehaviorTree->GetBlackboard()->GetData("housesInFOV", vHousesInFOV) == false || vHousesInFOV == nullptr)
	{
		return false;
	}
	vHousesInFOV->clear();
	HouseInfo hi = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV->push_back(hi);
			continue;
		}

		break;
	}

	return true;
}

bool Plugin::UpdateEntitiesInFOV()
{
	vector<EntityInfo>* vEntitiesInFOV{nullptr};
	if (m_pBehaviorTree->GetBlackboard()->GetData("entitiesInFOV", vEntitiesInFOV) == false || vEntitiesInFOV == nullptr)
	{
		return false;
	}
	vEntitiesInFOV->clear();
	EntityInfo ei = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, ei))
		{
			vEntitiesInFOV->push_back(ei);
			continue;
		}

		break;
	}
	
	return true;
}

vector<HouseInfo> Plugin::GetHousesInFOV() const
{
	vector<HouseInfo> vHousesInFOV = {};

	HouseInfo hi = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV.push_back(hi);
			continue;
		}

		break;
	}

	return vHousesInFOV;
}

vector<EntityInfo> Plugin::GetEntitiesInFOV() const
{
	vector<EntityInfo> vEntitiesInFOV = {};

	EntityInfo ei = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, ei))
		{
			vEntitiesInFOV.push_back(ei);
			continue;
		}

		break;
	}

	return vEntitiesInFOV;
}