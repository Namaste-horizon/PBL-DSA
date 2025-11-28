let users = JSON.parse(localStorage.getItem("users") || "[]");
let txs = JSON.parse(localStorage.getItem("txs") || "[]");

function saveUsers() { localStorage.setItem("users", JSON.stringify(users)); }

function saveTx() {
    localStorage.setItem("txs", JSON.stringify(txs));
    updateCategoryList();
}

function gentxid() { return "TX" + Math.random().toString(36).slice(2, 8); }

function updateCategoryList() {
    const dl = document.getElementById("catlist");
    if (!dl) return;
    const cats = Array.from(new Set(txs.map(t => (t.cat || "").trim()).filter(c => c)));
    dl.innerHTML = "";
    cats.forEach(c => {
        const opt = document.createElement("option");
        opt.value = c;
        dl.appendChild(opt);
    });
}

if (document.getElementById("createbtn")) {
    createbtn.onclick = () => {
        const n = newname.value.trim().toLowerCase();
        const p = newpass.value.trim();
        const q = question.value;
        const a = answer.value.trim().toLowerCase();
        if (!n || !p || !a) return alert("fill all fields");
        if (users.find(u => u.n === n)) return alert("username exists");
        users.push({ n, p, q, a });
        saveUsers();
        alert("account created");
        location.href = "index.html";
    };
}

if (document.getElementById("loginbtn")) {
    loginbtn.onclick = () => {
        const n = loginname.value.trim().toLowerCase();
        const p = loginpass.value.trim();
        const u = users.find(x => x.n === n && x.p === p);
        if (!u) return alert("invalid");
        localStorage.setItem("current", n);
        location.href = "dashboard.html";
    };
}

if (document.getElementById("showquestionbtn")) {
    let temp = null;
    showquestionbtn.onclick = () => {
        const name = fname.value.trim().toLowerCase();
        const u = users.find(x => x.n === name);
        if (!u) return alert("not found");
        const map = {
            '1': 'What is the name of your first pet?',
            '2': 'What is the first dish you learned to cook?',
            '3': 'What is your favorite book?',
            '4': 'What is the first word you said except mother and father?',
            '5': 'What city were you born in?'
        };
        showquestion.textContent = map[u.q];
        questionarea.classList.remove("hidden");
        temp = u;
    };
    resetbtn.onclick = () => {
        if (!temp) return;
        const ans = fanswer.value.trim().toLowerCase();
        const np = fnewpass.value.trim();
        if (ans !== temp.a) return alert("wrong answer");
        temp.p = np;
        saveUsers();
        alert("password updated");
        location.href = "index.html";
    };
}

function download(filename, text) {
    const a = document.createElement("a");
    const blob = new Blob([text], { type: "text/plain" });
    a.href = URL.createObjectURL(blob);
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(() => URL.revokeObjectURL(a.href), 1000);
}

function exportReportForUser(owner) {
    if (!owner) return alert("no user");
    const list = txs.filter(t => t.owner === owner);
    if (!list.length) {
        return download(`report_${owner}.csv`, "txid,date,category,description,amount\n# no transactions\n");
    }
    let out = "txid,date,category,description,amount\n";
    list.forEach(t => {
        const id = t.id || "";
        const date = t.date || "";
        const cat = t.cat || "";
        const det = (t.det || "").replace(/,/g, " ");
        const amt = (typeof t.amt === "number") ? t.amt.toFixed(2) : t.amt;
        out += `${id},${date},${cat},${det},${amt}\n`;
    });
    download(`report_${owner}.csv`, out);
}

function runFraudForUser(owner) {
    if (!owner) return alert("no user");
    const list = txs.filter(t => t.owner === owner);
    if (!list.length) {
        return download(`fraud_${owner}.txt`, `no data for user ${owner}\n`);
    }
    const results = [];
    for (let i = 0; i < list.length; ++i) {
        const a = list[i];
        const month = (a.date && a.date.length >= 7) ? a.date.slice(0, 7) : "";
        let sum = 0;
        let ccount = 0;
        for (let j = 0; j < list.length; ++j) {
            const b = list[j];
            const m2 = (b.date && b.date.length >= 7) ? b.date.slice(0, 7) : "";
            if (b.cat === a.cat && m2 === month) {
                sum += Number(b.amt || 0);
                ccount++;
            }
        }
        const avg = ccount > 0 ? sum / ccount : 0;
        if (avg > 0 && Number(a.amt) >= 3.0 * avg) {
            results.push(`[high spend warning] ${a.cat} ${a.date} ${Number(a.amt).toFixed(2)}`);
        }
    }
    if (!results.length) results.push("no fraud patterns");
    download(`fraud_${owner}.txt`, results.join("\n"));
}

if (document.getElementById("logout")) {
    const current = localStorage.getItem("current");
    if (!current) location.href = "index.html";
    currentuser.textContent = "Logged in: " + current;

    const navs = document.querySelectorAll(".nav");
    const views = document.querySelectorAll(".content");

    navs.forEach(n => n.onclick = () => {
        n.classList.add("active");
        views.forEach(v => v.classList.add("hidden"));
        document.getElementById(n.dataset.view).classList.remove("hidden");
        if (n.dataset.view === "reportview") render();
    });

    logout.onclick = () => {
        localStorage.removeItem("current");
        location.href = "index.html";
    };

    addrec.onclick = () => {
        const d = recdate.value.trim();
        const c = reccat.value.trim();
        const det = recdet.value.trim();
        const a = parseFloat(recamt.value);
        if (!d || !c || !det || isNaN(a)) return alert("fill all fields");
        const id = gentxid();
        txs.push({ id, owner: current, date: d, cat: c, det, amt: a });
        saveTx();
        lasttx.textContent = "Last TXID: " + id;
        recdate.value = reccat.value = recdet.value = recamt.value = "";
        render();
    };

    function render(sortKey) {
        const tb = document.getElementById("tbody");
        tb.innerHTML = "";
        let data = txs.filter(t => t.owner === current);
        if (sortKey) data.sort((a, b) => (a[sortKey] > b[sortKey] ? 1 : -1));
        data.forEach(t => {
            const tr = document.createElement("tr");
            tr.innerHTML = `
      <td>${t.id}</td>
      <td>${t.date}</td>
      <td>${t.cat}</td>
      <td>${t.det || "-"}</td>
      <td>${(Number(t.amt) || 0).toFixed(2)}</td>
    `;
            tb.appendChild(tr);
        });
        const sortbtn = document.getElementById("sortbtn");
        const sortselect = document.getElementById("sortselect");
        if (sortbtn) sortbtn.onclick = () => render(sortselect.value);
    }

    document.querySelectorAll("th[data-sort]").forEach(th => th.onclick = () => render(th.dataset.sort));

    runfraud.onclick = () => {
        const current = localStorage.getItem("current");
        if (!current) return location.href = "index.html";
        runFraudForUser(current);
    };
}

document.addEventListener("DOMContentLoaded", () => {
    const current = localStorage.getItem("current");
    if (!current) return;

    updateCategoryList();

    const splitmode = document.getElementById("ssplitmode");
    const custom = document.getElementById("scustom");

    if (splitmode) {
        splitmode.onchange = () => {
            if (splitmode.value === "custom") custom.classList.remove("hidden");
            else custom.classList.add("hidden");
        };
    }

    const splitbtn = document.getElementById("splitbtn");
    if (splitbtn) {
        splitbtn.onclick = () => {
            const d = sdate.value.trim();
            const c = scat.value.trim();
            const det = sdet.value.trim();
            const total = parseFloat(samt.value);
            const parts = sparticipants.value.split(",").map(x => x.trim().toLowerCase()).filter(x => x);
            if (!d || !c || !det || isNaN(total) || !parts.length) return alert("fill all fields");

            if (ssplitmode.value === "equal") {
                const each = +(total / parts.length).toFixed(2);
                parts.forEach(p => {
                    const id = gentxid();
                    txs.push({ id, owner: p, date: d, cat: c, det, amt: each });
                });
            } else {
                const customvals = scustom.value.split(",").map(x => parseFloat(x.trim())).filter(x => !isNaN(x));
                if (customvals.length !== parts.length) return alert("custom amounts count must match participants");
                for (let i = 0; i < parts.length; ++i) {
                    const id = gentxid();
                    txs.push({ id, owner: parts[i], date: d, cat: c, det, amt: customvals[i] });
                }
            }

            saveTx();
            splittx.textContent = "split added";
            sdate.value = scat.value = sdet.value = samt.value = sparticipants.value = scustom.value = "";

            const activeNav = document.querySelector(".nav.active");
            if (activeNav && activeNav.dataset.view === "reportview") render();
        };
    }

    const exportbtn = document.createElement("button");
    exportbtn.id = "exportreportbtn";
    exportbtn.textContent = "Export Report";
    exportbtn.className = "btn";

    const top = document.querySelector("#reportview > h2");
    if (top && top.parentNode) top.parentNode.insertBefore(exportbtn, top.nextSibling);

    exportbtn.onclick = () => {
        const current = localStorage.getItem("current");
        if (!current) return location.href = "index.html";
        exportReportForUser(current);
    };
});
