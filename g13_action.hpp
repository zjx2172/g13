//
// Created by khampf on 07-05-2020.
//

#ifndef G13_G13_ACTION_HPP
#define G13_G13_ACTION_HPP

#include <memory>
#include <vector>

namespace  G13 {

    class G13_Device;
    class G13_Manager;

    typedef int LINUX_KEY_VALUE;
    const LINUX_KEY_VALUE BAD_KEY_VALUE = -1;

    // typedef std::shared_ptr<G13_Profile> ProfilePtr;
    // class G13_Manager;

    // *************************************************************************

/*! holds potential actions which can be bound to G13 activity
 *
 */
    class G13_Action {
    public:
        explicit G13_Action(G13_Device& keypad) : _keypad(keypad) {}
        virtual ~G13_Action();

        virtual void act(G13_Device&, bool is_down) = 0;
        virtual void dump(std::ostream&) const = 0;

        void act(bool is_down) { act(keypad(), is_down); }

        G13_Device& keypad() { return _keypad; }

        // [[maybe_unused]] [[nodiscard]] const G13_Device& keypad() const { return _keypad; }

        G13_Manager& manager();
        [[nodiscard]] const G13_Manager& manager() const;

    private:
        G13_Device& _keypad;
    };

    typedef std::shared_ptr<G13_Action> G13_ActionPtr;

/*!
 * action to send one or more keystrokes
 */
    class G13_Action_Keys : public G13_Action {
    public:
        G13_Action_Keys(G13_Device& keypad, const std::string& keys);
        ~G13_Action_Keys() override;

        void act(G13_Device&, bool is_down) override;
        void dump(std::ostream&) const override;

        std::vector<LINUX_KEY_VALUE> _keys;
    };

/*!
 * action to send a string to the output pipe
 */
    class G13_Action_PipeOut : public G13_Action {
    public:
        G13_Action_PipeOut(G13_Device& keypad, const std::string& out);
        ~G13_Action_PipeOut() override;

        void act(G13_Device&, bool is_down) override;
        void dump(std::ostream&) const override;

        std::string _out;
    };

/*!
 * action to send a command to the g13
 */
    class G13_Action_Command : public G13_Action {
    public:
        G13_Action_Command(G13_Device& keypad, std::string cmd);
        ~G13_Action_Command() override;

        void act(G13_Device&, bool is_down) override;
        void dump(std::ostream&) const override;

        std::string _cmd;
    };

// *************************************************************************
    template <class PARENT_T>
    class G13_Actionable {
    public:
        G13_Actionable(PARENT_T& parent_arg, std::string  name)
                : _parent_ptr(&parent_arg), _name(std::move(name)) {}
        virtual ~G13_Actionable() { _parent_ptr = nullptr; }

        [[nodiscard]] G13_ActionPtr action() const { return _action; }
        [[nodiscard]] const std::string& name() const { return _name; }
        // PARENT_T& parent() { return *_parent_ptr; }
        // [[nodiscard]] const PARENT_T& parent() const { return *_parent_ptr; }
        // G13_Manager& manager() { return _parent_ptr->manager(); }
        [[nodiscard]] const G13_Manager& manager() const { return _parent_ptr->manager(); }

        virtual void set_action(const G13_ActionPtr& action) { _action = action; }

    protected:
        std::string _name;
        G13_ActionPtr _action;

    private:
        PARENT_T* _parent_ptr;
    };
}
#endif //G13_G13_ACTION_HPP
