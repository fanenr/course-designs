import util, psycopg

from ext import pwd, conn
from flask import request
from flask import Blueprint

from psycopg.rows import dict_row
from flask_jwt_extended import jwt_required
from flask_jwt_extended import get_jwt_identity
from flask_jwt_extended import create_access_token


bp = Blueprint("student", __name__, url_prefix="/student")


@bp.route("/register", methods=["POST"])
def register():
    data = request.form
    if not util.check(data, "name", "start", "username", "password"):
        return util.badreq("The parameters are incomplete")

    try:
        sql = "INSERT INTO student (name, start, username, password) VALUES (%s, %s, %s, %s)"
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
        sql = "SELECT id, name, start, password FROM student WHERE username = %s"
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


@bp.route("/course/list", methods=["GET"])
@jwt_required(optional=False)
def course_list():
    try:
        sql = """SELECT c.id AS id, c.name AS name, c.start AS start, t.name AS teacher
                 FROM course AS c JOIN teacher AS t ON c.tid = t.id"""
        with conn.transaction():
            cur = conn.cursor(row_factory=dict_row)
            cur.execute(sql)
    except Exception:
        return util.internal("Internal error")

    all = cur.fetchall()

    cur.close()
    return all


@bp.route("/course/take", methods=["POST"])
@jwt_required(optional=False)
def course_take():
    data = request.form
    if not util.check(data, "cid"):
        return util.badreq("The parameters are incomplete")

    try:
        sql = "INSERT INTO grade (sid, cid) VALUES (%s, %s)"
        value = (get_jwt_identity(), int(data["cid"]))
        with conn.transaction():
            cur = conn.cursor()
            cur.execute(sql, value)
    except psycopg.IntegrityError:
        return util.conflict("Have taken the course")
    except Exception:
        return util.internal("Internal error")

    cur.close()
    return "ok"


@bp.route("/grade/list", methods=["GET"])
@jwt_required(optional=False)
def grade_list():
    try:
        sql = """SELECT g.id AS id, score, c.name AS course, t.name AS name
                 FROM grade AS g JOIN course AS c ON cid = c.id
                 JOIN teacher AS t ON tid = t.id
                 WHERE sid = %s"""
        value = (get_jwt_identity(),)
        with conn.transaction():
            cur = conn.cursor(row_factory=dict_row)
            cur.execute(sql, value)
    except Exception:
        return util.internal("Internal error")

    all = cur.fetchall()

    cur.close()
    return all
