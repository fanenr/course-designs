def check(data, *fields):
    for f in fields:
        if not data.get(f):
            return False
    return True


def badreq(msg):
    return msg, 400


def unauth(msg):
    return msg, 401


def conflict(msg):
    return msg, 409


def internal(msg):
    return msg, 500
