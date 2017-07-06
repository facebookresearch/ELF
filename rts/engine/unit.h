/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#ifndef _UNIT_H_
#define _UNIT_H_

#include "player.h"
#include "map.h"
#include "cmd.h"
#include <initializer_list>
#include <assert.h>

// ---------------------------------------------------- Unit ----------------------------------------------
//
class Unit {
protected:
  // Basic Attributes.
  UnitId _id;
  // string _name;
  UnitType _type;
  PointF _p, _last_p;
  Tick _built_since;
  UnitProperty _property;

public:
  Unit() : Unit(INVALID, INVALID, WORKER, PointF(), UnitProperty()) {
  }

  Unit(Tick tick, UnitId id, UnitType t, const PointF &p, const UnitProperty& property)
      : _id(id), _type(t), _p(p), _last_p(p), _built_since(tick), _property(property) {
  }

  UnitId GetId() const { return _id; }
  PlayerId GetPlayerId() const;
  const PointF &GetPointF() const { return _p; }
  const PointF &GetLastPointF() const { return _last_p; }
  void SetPointF(const PointF &p) { _last_p = _p; _p = p; }

  Tick GetBuiltSince() const { return _built_since; }

  UnitType GetUnitType() const { return _type; }
  const UnitProperty &GetProperty() const { return _property; }
  UnitProperty &GetProperty() { return _property; }

  // Visualization. Output a vector of string as the visual command.
  string Draw(Tick tick) const;

  // Print info in the screen.
  string PrintInfo(const RTSMap &m) const;

  SERIALIZER(Unit, _id, _type, _p, _last_p, _built_since, _property);
  HASH(Unit, _property, _id, _type, _p, _last_p, _built_since);

  bool HasFlag() const {return _property._has_flag == 1;}
};

STD_HASH(Unit);

typedef map<UnitId, unique_ptr<Unit> > Units;

#endif
