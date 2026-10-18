const STATUSES = ["Booked", "Received", "Inspection", "In Progress", "Ready for Pickup", "Completed", "Cancelled"];
const PRIORITY = {Urgent: 3, High: 2, Normal: 1};
let records = [];
let queue = [];
let toastTimer;
let openMenuId = null;

async function api(path, method = "GET", data = null) {
  const options = {method, headers: {"Content-Type": "application/json"}};
  if (data !== null) options.body = JSON.stringify(data);
  const response = await fetch(path, options);
  const payload = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(payload.error || `Request failed (${response.status})`);
  return payload;
}
async function refresh() {
  try {
    const [serviceData, queueData] = await Promise.all([api("/api/services"), api("/api/queue")]);
    records = serviceData;
    queue = queueData;
    render();
  } catch (error) {
    toast("Cannot connect to C backend. Run backend\\server.exe and open http://localhost:8080");
    console.error(error);
  }
}
function esc(v) {
  return String(v ?? "").replace(/[&<>"']/g, c => ({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[c]));
}
function nextId() { return records.reduce((m, r) => Math.max(m, Number(r.id) || 1000), 1000) + 1; }
function toast(message) {
  const el = document.getElementById("toast");
  el.textContent = message; el.classList.add("show");
  clearTimeout(toastTimer); toastTimer = setTimeout(() => el.classList.remove("show"), 2600);
}
function statusClass(status) { return "s-" + status.toLowerCase().replaceAll(" ", "-"); }
function formatDate(value) {
  if (!value) return "Not specified";
  const d = new Date(value + (value.length === 10 ? "T00:00:00" : ""));
  return Number.isNaN(d.getTime()) ? value : d.toLocaleDateString(undefined, {day:"numeric",month:"short",year:"numeric"});
}
function updateStats() {
  document.getElementById("totalCount").textContent = records.length;
  document.getElementById("activeCount").textContent = records.filter(r => !["Completed","Cancelled"].includes(r.status)).length;
  document.getElementById("completedCount").textContent = records.filter(r => r.status === "Completed").length;
  document.getElementById("queueCount").textContent = queue.length;
  document.getElementById("totalHint").textContent = records.length ? `${records.filter(r => r.status === "Booked").length} newly booked` : "Across all bookings";
}
function filteredRecords() {
  const q = document.getElementById("searchInput").value.trim().toLowerCase();
  const status = document.getElementById("statusFilter").value;
  const sort = document.getElementById("sortSelect").value;
  const list = records.filter(r => {
    const matches = [r.id, `VS-${r.id}`, r.customer, r.vehicle, r.serviceType, r.phone].some(v => String(v || "").toLowerCase().includes(q));
    return matches && (status === "All" || r.status === status);
  });
  list.sort((a,b) => {
    if (sort === "oldest") return a.id - b.id;
    if (sort === "customer") return a.customer.localeCompare(b.customer);
    if (sort === "priority") return (PRIORITY[b.priority] || 0) - (PRIORITY[a.priority] || 0) || b.id - a.id;
    return b.id - a.id;
  });
  return list;
}
function renderTable() {
  const tbody = document.getElementById("serviceTable");
  const visible = filteredRecords();
  document.getElementById("emptyState").classList.toggle("show", visible.length === 0);
  tbody.innerHTML = visible.map(r => `<tr>
    <td><div class="vehicle-cell"><span class="vehicle-mini">${r.vehicleType === "Motorcycle" ? "🏍" : "🚘"}</span><div><strong>${esc(r.vehicle)}</strong><small>${esc(r.customer)}</small></div></div></td>
    <td><div class="service-type-cell" title="${esc(r.serviceType)}">${esc(r.serviceType)}</div></td>
    <td><span class="status ${statusClass(r.status)}">${esc(r.status)}</span></td>
    <td><span class="tracking-code">VS-${r.id}</span></td>
    <td><div class="action-wrap"><button class="action-btn" data-menu="${r.id}" aria-label="Actions for VS-${r.id}">•••</button><div class="action-menu ${openMenuId === r.id ? "open" : ""}">
      <button data-view="${r.id}">View details</button>
      <button data-queue="${r.id}" ${queue.includes(r.id) || ["Completed","Cancelled"].includes(r.status) ? "disabled" : ""}>${queue.includes(r.id) ? "Already queued" : "Add to queue"}</button>
      <button data-next="${r.id}" ${r.status === "Completed" || r.status === "Cancelled" ? "disabled" : ""}>Advance status</button>
      <button class="danger" data-delete="${r.id}">Delete record</button>
    </div></div></td>
  </tr>`).join("");
  document.getElementById("recordSummary").textContent = `Showing ${visible.length} of ${records.length} record${records.length === 1 ? "" : "s"}`;
}
function renderQueue() {
  const container = document.getElementById("queueList");
  const activeQueue = queue.map(id => records.find(r => r.id === id)).filter(r => r && !["Completed","Cancelled"].includes(r.status));
  container.innerHTML = activeQueue.length ? activeQueue.map((r,i) => `<div class="queue-item"><span class="queue-position">${i+1}</span><div class="queue-item-main"><strong>${esc(r.vehicle)} · ${esc(r.customer)}</strong><small>VS-${r.id} &nbsp;·&nbsp; ${esc(r.serviceType)}</small></div><span class="queue-time">${esc(r.priority || "Normal")} priority</span></div>`).join("") : `<div class="queue-empty">No vehicles waiting. Add a service record to the queue from the actions menu.</div>`;
  document.getElementById("processQueueBtn").disabled = activeQueue.length === 0;
  document.getElementById("processQueueBtn").style.opacity = activeQueue.length === 0 ? ".55" : "1";
}
function render() { updateStats(); renderTable(); renderQueue(); }
function createRecord(form) {
  const data = new FormData(form);
  return {
    customer: String(data.get("customer") || "").trim(),
    phone: String(data.get("phone") || "").trim(),
    vehicle: String(data.get("vehicle") || "").trim().toUpperCase(),
    vehicleType: String(data.get("vehicleType") || "Car"),
    serviceType: String(data.get("serviceType") || ""),
    date: String(data.get("date") || ""),
    priority: String(data.get("priority") || "Normal"),
    issue: String(data.get("issue") || "").trim(),
    status: "Booked", createdAt: new Date().toISOString()
  };
}
document.getElementById("bookingForm").addEventListener("submit", async e => {
  e.preventDefault();
  const record = createRecord(e.currentTarget);
  if (!record.customer || !record.vehicle || !record.serviceType) return toast("Please fill in all required fields.");
  if (records.some(r => r.vehicle === record.vehicle && !["Completed","Cancelled"].includes(r.status))) {
    if (!confirm("There is already an active service for this vehicle. Create another booking anyway?")) return;
  }
  try {
    const created = await api("/api/services", "POST", record);
    e.currentTarget.reset(); await refresh();
    toast(`Booking created successfully · Tracking ID VS-${created.id}`);
    document.getElementById("services").scrollIntoView({behavior:"smooth", block:"start"});
  } catch (error) { toast(error.message); }
});
["searchInput","statusFilter","sortSelect"].forEach(id => document.getElementById(id).addEventListener(id === "searchInput" ? "input" : "change", renderTable));
document.getElementById("serviceTable").addEventListener("click", async e => {
  const btn = e.target.closest("button"); if (!btn) return;
  const id = Number(btn.dataset.menu || btn.dataset.view || btn.dataset.queue || btn.dataset.next || btn.dataset.delete);
  if (!id) return;
  const record = records.find(r => Number(r.id) === id); if (!record) return;
  if (btn.dataset.menu) { openMenuId = openMenuId === id ? null : id; renderTable(); return; }
  if (btn.dataset.view) { openDetails(record); openMenuId = null; renderTable(); return; }
  if (btn.dataset.queue) {
    try { await api("/api/queue", "POST", {id}); await refresh(); toast(`VS-${id} added to the FIFO waiting queue.`); }
    catch (error) { toast(error.message); }
    openMenuId = null; renderTable(); return;
  }
  if (btn.dataset.next) {
    const current = STATUSES.indexOf(record.status);
    const nextStatus = STATUSES[Math.min(current + 1, STATUSES.indexOf("Completed"))];
    try { await api(`/api/services/${id}/status`, "PATCH", {status: nextStatus}); await refresh(); toast(`VS-${id} status changed to ${nextStatus}.`); }
    catch (error) { toast(error.message); }
    openMenuId = null; return;
  }
  if (btn.dataset.delete && confirm(`Delete service record VS-${id}? This cannot be undone.`)) {
    try { await api(`/api/services/${id}`, "DELETE"); await refresh(); toast(`VS-${id} deleted.`); }
    catch (error) { toast(error.message); }
    openMenuId = null;
  }
});
document.addEventListener("click", e => {
  if (!e.target.closest(".action-wrap") && openMenuId !== null) { openMenuId = null; renderTable(); }
});
document.getElementById("processQueueBtn").addEventListener("click", async () => {
  try {
    const record = await api("/api/queue/process", "POST", {});
    await refresh(); toast(`Processed next in queue: VS-${record.id} is now ${record.status}.`);
  } catch (error) { toast(error.message === "Queue is empty" ? "No active service in the queue." : error.message); await refresh(); }
});
document.getElementById("exportBtn").addEventListener("click", () => {
  const cols = ["id","customer","phone","vehicle","vehicleType","serviceType","date","priority","issue","status","createdAt"];
  const quote = v => `"${String(v ?? "").replaceAll('"','""')}"`;
  const csv = [cols.join(","), ...records.map(r => cols.map(k => quote(r[k])).join(","))].join("\r\n");
  const blob = new Blob([csv], {type:"text/csv;charset=utf-8;"});
  const url = URL.createObjectURL(blob); const a = document.createElement("a");
  a.href = url; a.download = "autocare-service-records.csv"; a.click(); URL.revokeObjectURL(url);
  toast("Service records exported to CSV.");
});
document.getElementById("clearDemoBtn").addEventListener("click", async () => {
  if (!records.length) return toast("There are no service records to clear.");
  if (confirm("Delete all service records and clear the queue?")) {
    try { await api("/api/services", "DELETE"); await refresh(); toast("All service records cleared."); }
    catch (error) { toast(error.message); }
  }
});
function openDetails(r) {
  document.getElementById("detailTitle").textContent = `VS-${r.id} · ${r.vehicle}`;
  document.getElementById("detailContent").innerHTML = `<div class="detail-grid">
    <div class="detail-field"><small>Customer</small><strong>${esc(r.customer)}</strong></div>
    <div class="detail-field"><small>Phone</small><strong>${esc(r.phone || "Not provided")}</strong></div>
    <div class="detail-field"><small>Vehicle type</small><strong>${esc(r.vehicleType)}</strong></div>
    <div class="detail-field"><small>Service type</small><strong>${esc(r.serviceType)}</strong></div>
    <div class="detail-field"><small>Preferred date</small><strong>${esc(formatDate(r.date))}</strong></div>
    <div class="detail-field"><small>Priority / status</small><strong>${esc(r.priority)} · ${esc(r.status)}</strong></div>
    <div class="detail-field"><small>Booking created</small><strong>${esc(new Date(r.createdAt).toLocaleString())}</strong></div>
    <div class="detail-field"><small>Queue position</small><strong>${queue.includes(r.id) ? queue.indexOf(r.id)+1 : "Not queued"}</strong></div>
  </div><div class="detail-description"><strong>Issue description</strong><br>${esc(r.issue || "No issue description provided.")}</div>`;
  document.getElementById("detailDialog").showModal();
}
["closeDialogBtn","closeDialogBtn2"].forEach(id => document.getElementById(id).addEventListener("click", () => document.getElementById("detailDialog").close()));
document.getElementById("themeBtn").addEventListener("click", () => {
  document.body.classList.toggle("dark");
  document.getElementById("themeBtn").textContent = document.body.classList.contains("dark") ? "☾" : "☼";
});
document.addEventListener("keydown", e => {
  if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "k") { e.preventDefault(); document.getElementById("searchInput").focus(); }
});
refresh();
