#pragma once

#include <protobuf/referee.pb.h>
#include "TeamInfo.hpp"

#include <QThread>
#include <QMutex>

#include <vector>

#include <stdint.h>

class QUdpSocket;

namespace NewRefereeModuleEnums
{
// These are the "coarse" stages of the game.
enum Stage {
	// The first half is about to start.
	// A kickoff is called within this stage.
	// This stage ends with the NORMAL_START.
	NORMAL_FIRST_HALF_PRE = 0,
	// The first half of the normal game, before half time.
	NORMAL_FIRST_HALF = 1,
	// Half time between first and second halves.
	NORMAL_HALF_TIME = 2,
	// The second half is about to start.
	// A kickoff is called within this stage.
	// This stage ends with the NORMAL_START.
	NORMAL_SECOND_HALF_PRE = 3,
	// The second half of the normal game, after half time.
	NORMAL_SECOND_HALF = 4,
	// The break before extra time.
	EXTRA_TIME_BREAK = 5,
	// The first half of extra time is about to start.
	// A kickoff is called within this stage.
	// This stage ends with the NORMAL_START.
	EXTRA_FIRST_HALF_PRE = 6,
	// The first half of extra time.
	EXTRA_FIRST_HALF = 7,
	// Half time between first and second extra halves.
	EXTRA_HALF_TIME = 8,
	// The second half of extra time is about to start.
	// A kickoff is called within this stage.
	// This stage ends with the NORMAL_START.
	EXTRA_SECOND_HALF_PRE = 9,
	// The second half of extra time.
	EXTRA_SECOND_HALF = 10,
	// The break before penalty shootout.
	PENALTY_SHOOTOUT_BREAK = 11,
	// The penalty shootout.
	PENALTY_SHOOTOUT = 12,
	// The game is over.
	POST_GAME = 13
};

std::string stringFromStage(Stage s);

// These are the "fine" states of play on the field.
enum Command {
	// All robots should completely stop moving.
	HALT = 0,
	// Robots must keep 50 cm from the ball.
	STOP = 1,
	// A prepared kickoff or penalty may now be taken.
	NORMAL_START = 2,
	// The ball is dropped and free for either team.
	FORCE_START = 3,
	// The yellow team may move into kickoff position.
	PREPARE_KICKOFF_YELLOW = 4,
	// The blue team may move into kickoff position.
	PREPARE_KICKOFF_BLUE = 5,
	// The yellow team may move into penalty position.
	PREPARE_PENALTY_YELLOW = 6,
	// The blue team may move into penalty position.
	PREPARE_PENALTY_BLUE = 7,
	// The yellow team may take a direct free kick.
	DIRECT_FREE_YELLOW = 8,
	// The blue team may take a direct free kick.
	DIRECT_FREE_BLUE = 9,
	// The yellow team may take an indirect free kick.
	INDIRECT_FREE_YELLOW = 10,
	// The blue team may take an indirect free kick.
	INDIRECT_FREE_BLUE = 11,
	// The yellow team is currently in a timeout.
	TIMEOUT_YELLOW = 12,
	// The blue team is currently in a timeout.
	TIMEOUT_BLUE = 13,
	// The yellow team just scored a goal.
	// For information only.
	// For rules compliance, teams must treat as STOP.
	GOAL_YELLOW = 14,
	// The blue team just scored a goal.
	GOAL_BLUE = 15
};

std::string stringFromCommand(Command c);
}

class NewRefereePacket
{
public:
	/// Local time when the packet was received
	uint64_t receivedTime;

	/// protobuf message from the vision system
	SSL_Referee wrapper;
};

class NewRefereeModule: public QThread
{
public:
	NewRefereeModule();
	~NewRefereeModule();

	void stop();

	void getPackets(std::vector<NewRefereePacket *> &packets);

	NewRefereeModuleEnums::Stage stage;
	NewRefereeModuleEnums::Command command;

	// The UNIX timestamp when the packet was sent, in microseconds.
	// Divide by 1,000,000 to get a time_t.
	uint64_t sent_time;

	// The number of microseconds left in the stage.
	// The following stages have this value; the rest do not:
	// NORMAL_FIRST_HALF
	// NORMAL_HALF_TIME
	// NORMAL_SECOND_HALF
	// EXTRA_TIME_BREAK
	// EXTRA_FIRST_HALF
	// EXTRA_HALF_TIME
	// EXTRA_SECOND_HALF
	// PENALTY_SHOOTOUT_BREAK
	//
	// If the stage runs over its specified time, this value
	// becomes negative.
	int stage_time_left;

	// The number of commands issued since startup (mod 2^32).
	uint command_counter;

	// The UNIX timestamp when the command was issued, in microseconds.
	// This value changes only when a new command is issued, not on each packet.
	uint64_t command_timestamp;

	TeamInfo yellow_info;
	TeamInfo blue_info;

protected:
	virtual void run();

	volatile bool _running;

	QMutex _mutex;
	std::vector<NewRefereePacket *> _packets;
};