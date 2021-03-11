//
// Autogenerated by Thrift Compiler (0.13.0)
//
// DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
//
import thrift = require('thrift');
import Thrift = thrift.Thrift;
import Q = thrift.Q;
import Int64 = require('node-int64');


declare enum MESSAGE_TYPE {
  LOG = 1,
  ERROR = 2,
  STOP = 3,
}

declare class ProjectInfo {
    public name: string;
    public feature: string;
    public version: Int64;

      constructor(args?: { name: string; feature: string; version: Int64; });
  }

declare class Message {
    public type: MESSAGE_TYPE;
    public message: string;
    public path: string;
    public line: number;

      constructor(args?: { type: MESSAGE_TYPE; message: string; path: string; line: number; });
  }
