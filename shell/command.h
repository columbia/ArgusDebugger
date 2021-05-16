#ifndef SHELL_COMMAND_H
#define SHELL_COMMAND_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include "loguru.hpp"

class Arguments {
private:
    typedef uint64_t address_t;
    std::vector<std::string> args;
public:
    Arguments popFront();
    void add(const std::string &arg) { args.push_back(arg); }
    bool asHex(std::size_t index, address_t *address);
    bool asDec(std::size_t index, address_t *value);
    void shouldHave(std::size_t count);
    std::size_t size() const { return args.size(); }
    std::string front() const { return args.front(); }
    std::string get(size_t i) const { return (i < args.size() ? args[i] : ""); }

	uint64_t get_num(size_t i, std::string hex) {
		uint64_t ret;
		if (hex == "hex" && asHex(i, &ret))
			return ret;
		if (hex == "dec" && asDec(i, &ret))
			return ret;
		LOG_S(INFO) << "invalid arg " << std::dec << i << std::endl;
		return -1;
	}

    std::vector<std::string>::iterator begin() { return args.begin(); }
    std::vector<std::string>::const_iterator begin() const { return args.begin(); }
    std::vector<std::string>::iterator end() { return args.end(); }
    std::vector<std::string>::const_iterator end() const { return args.end(); }
};

class Command {
public:
    virtual ~Command() {}
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;
    virtual void operator () (Arguments args) = 0;
};

class CommandImpl : public Command {
private:
    std::string name;
    std::string desc;
public:
    CommandImpl(const std::string &name, const std::string &desc)
        : name(name), desc(desc) {}
    virtual std::string getName() const { return name; }
    virtual std::string getDescription() const { return desc; }
};

class FunctionCommand : public CommandImpl {
public:
    typedef std::function<void (Arguments)> FunctionType;
private:
    FunctionType func;
public:
    FunctionCommand(const std::string &name, const FunctionType &func,
        const std::string &desc = "")
        : CommandImpl(name, desc), func(func) {}
    virtual void operator () (Arguments args) { func(args); }
};

class CommandList : public CommandImpl {
public:
    typedef std::map<std::string, Command *> CommandMapType;
private:
    CommandMapType commandMap;
public:
    using CommandImpl::CommandImpl;
    virtual ~CommandList() {}

    CommandMapType &getMap() { return commandMap; }

    void add(Command *command) { commandMap[command->getName()] = command; }
    void add(std::string command, const FunctionCommand::FunctionType &func, const std::string &desc = "")
        { commandMap[command] = new FunctionCommand(command, func, desc); }
    virtual void operator () (Arguments args) = 0;
};

class CompositeCommand : public CommandList {
public:
    using CommandList::CommandList;
    virtual void operator () (Arguments args);
    virtual void invokeNull(Arguments args) {LOG_S(INFO) << "Null";}
    virtual void invokeDefault(Arguments args) {LOG_S(INFO) << "Default";}
};
#endif
