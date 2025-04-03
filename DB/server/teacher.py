import util, psycopg

from ext import pwd, conn
from flask import request
from flask import Blueprint

from psycopg.rows import dict_row
from flask_jwt_extended import jwt_required
from flask_jwt_extended import get_jwt_identity
from flask_jwt_extended import create_access_token


bp = Blueprint("teacher", __name__, url_prefix="/teacher")


@bp.route("/register", methods=["POST"])
def register():
    data = request.form
    if not util.check(data, "name", "start", "username", "password"):
        return util.badreq("The parameters are incomplete")

    try:
        sql = "INSERT INTO teacher (name, start, username, password) VALUES (%s, %s, %s, %s)"
        value = (
            data["name"],
            data["start"],
            data["username"],
            pwd.hash(data["password"]),
        )
        with conn.transaction():
            cur = conn.cursor()
            cur.execute(sql, value)
    except psycopg.IntegrityError:
        return util.conflict("The user already exists")
    except Exception:
        return util.internal("Internal error")

    cur.close()
    return "ok"


@bp.route("/login", methods=["POST"])
def login():
    data = request.form
    if not util.check(data, "username", "password"):
        return util.badreq("The parameters are incomplete")

    try:
        sql = "SELECT id, name, start, password FROM teacher WHERE username = %s"
        value = (data["username"],)
        with conn.transaction():
            cur = conn.cursor(row_factory=dict_row)
            cur.execute(sql, value)
    except Exception:
        return util.internal("Internal error")

    if not (info := cur.fetchone()):
        return util.unauth("The user does not exist")
    if not pwd.verify(data["password"], info["password"]):
        return util.unauth("Wrong username or password")

    token = create_access_token(info["id"])
    info["token"] = token
    del info["password"]

    cur.close()
    return info


@bp.route("/course/create", methods=["POST"])
@jwt_required(optional=False)
def course_create():
    data = request.form
    if not util.check(data, "name", "start"):
        return util.badreq("The parameters are incomplete")

    try:
        sql = "INSERT INTO course (tid, name, start) VALUES (%s, %s, %s)"
        value = (get_jwt_identity(), data["name"], data["start"])
        with conn.transaction():
            cur = conn.cursor()
            cur.execute(sql, value)
    except psycopg.IntegrityError:
        return util.conflict("The course already exists")
    except Exception:
        return util.internal("Internal error")

    cur.close()
    return "ok"


@bp.route("/course/list", methods=["GET"])
@jwt_required(optional=False)
def course_list():
    try:
        sql = "SELECT id, name, start FROM course WHERE tid = %s"
        value = (get_jwt_identity(),)
        with conn.transaction():
            cur = conn.cursor(row_factory=dict_row)
            cur.execute(sql, value)
    except Exception:
        return util.internal("Internal error")

    all = cur.fetchall()

    cur.close()
    return all


@bp.route("/grade/update", methods=["POST"])
@jwt_required(optional=False)
def grade_update():
    data = request.form
    if not util.check(data, "gid", "score"):
        return util.badreq("The parameters are incomplete")

    try:
        sql = "UPDATE grade SET score = %s WHERE id = %s"
        value = (int(data["score"]), int(data["gid"]))
        with conn.transaction():
            cur = conn.cursor()
            cur.execute(sql, value)
    except Exception:
        return util.internal("Internal error")

    row = cur.rowcount
    cur.close()

    if row != 1:
        return util.conflict("The course or student does not exist")
    return "ok"


@bp.route("/grade/list", methods=["GET"])
@jwt_required(optional=False)
def grade_list():
    try:
        sql = """SELECT g.id AS id, score, c.name AS course, s.name AS name
                 FROM grade AS g JOIN course AS c ON g.cid = c.id
                 JOIN student AS s ON g.sid = s.id
                 WHERE tid = %s"""
        value = (get_jwt_identity(),)
        with conn.transaction():
            cur = conn.cursor(row_factory=dict_row)
            cur.execute(sql, value)
    except Exception:
        return util.internal("Internal error")

    all = cur.fetchall()

    cur.close()
    return all
