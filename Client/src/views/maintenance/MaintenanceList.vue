<template>
  <div class="page-container">
    <div class="page-header">
      <h2>Maintenance</h2>
      <p>Track vehicle maintenance records</p>
    </div>

    <div class="search-bar">
      <el-select v-model="query.status" placeholder="All Status" clearable style="width: 140px" @change="loadList">
        <el-option label="Pending" value="pending" />
        <el-option label="In Progress" value="in_progress" />
        <el-option label="Completed" value="completed" />
      </el-select>
      <el-select v-model="query.type" placeholder="All Types" clearable style="width: 140px" @change="loadList">
        <el-option label="Regular" value="regular" />
        <el-option label="Repair" value="repair" />
        <el-option label="Inspection" value="inspection" />
        <el-option label="Emergency" value="emergency" />
      </el-select>
      <el-button type="primary" @click="loadList">
        <el-icon><Search /></el-icon> Search
      </el-button>
      <el-button type="primary" @click="$router.push('/maintenance/add')" style="margin-left: auto">
        <el-icon><Plus /></el-icon> Add Record
      </el-button>
    </div>

    <el-card>
      <el-table :data="records" v-loading="loading" style="width: 100%" empty-text="No records found">
        <el-table-column prop="vehicle_plate" label="Vehicle" min-width="120" />
        <el-table-column prop="type" label="Type" min-width="100">
          <template #default="{ row }">
            <el-tag>{{ row.type }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="description" label="Description" min-width="180" show-overflow-tooltip />
        <el-table-column prop="cost" label="Cost" min-width="100">
          <template #default="{ row }">¥{{ row.cost || 0 }}</template>
        </el-table-column>
        <el-table-column prop="technician" label="Technician" min-width="100" />
        <el-table-column prop="start_date" label="Start Date" min-width="110" />
        <el-table-column prop="status" label="Status" min-width="110">
          <template #default="{ row }">
            <el-tag :type="maintStatusType(row.status)">{{ row.status }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column label="Actions" min-width="100">
          <template #default="{ row }">
            <el-button link type="primary" @click="$router.push(`/maintenance/${row.id}/edit`)">Edit</el-button>
          </template>
        </el-table-column>
      </el-table>

      <div class="pagination-wrapper" v-if="total > query.page_size">
        <el-pagination
          background
          layout="prev, pager, next"
          :total="total"
          :page-size="query.page_size"
          :current-page="query.page"
          @current-change="handlePageChange"
        />
      </div>
    </el-card>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { getMaintenanceList } from '@/api/maintenance'
import { ElMessage } from 'element-plus'
import { Search, Plus } from '@element-plus/icons-vue'

const loading = ref(false)
const records = ref([])
const total = ref(0)

const query = reactive({
  page: 1,
  page_size: 20,
  status: '',
  type: '',
})

function maintStatusType(status) {
  const map = {
    'pending': 'warning',
    'in_progress': 'primary',
    'completed': 'success',
  }
  return map[(status || '').toLowerCase()] || 'info'
}

async function loadList() {
  loading.value = true
  try {
    const params = { ...query }
    if (!params.status) delete params.status
    if (!params.type) delete params.type
    const res = await getMaintenanceList(params)
    if (res.error === 0) {
      records.value = res.list || res.records || []
      total.value = res.total || 0
    }
  } catch {
    ElMessage.error('Failed to load maintenance records')
  } finally {
    loading.value = false
  }
}

function handlePageChange(page) {
  query.page = page
  loadList()
}

onMounted(loadList)
</script>

<style scoped>
.search-bar {
  display: flex;
  gap: 12px;
  align-items: center;
  margin-bottom: 20px;
  flex-wrap: wrap;
}

.pagination-wrapper {
  display: flex;
  justify-content: center;
  margin-top: 20px;
}
</style>
