<template>
  <div class="page-container">
    <div class="page-header">
      <h2>Vehicles</h2>
      <p>Manage vehicle inventory</p>
    </div>

    <!-- Search Bar -->
    <div class="search-bar">
      <el-input
        v-model="query.keyword"
        placeholder="Search vehicles..."
        :prefix-icon="Search"
        clearable
        style="width: 240px"
        @keyup.enter="loadVehicles"
        @clear="loadVehicles"
      />
      <el-select v-model="query.status" placeholder="All Status" clearable style="width: 140px" @change="loadVehicles">
        <el-option label="Available" value="available" />
        <el-option label="Rented" value="rented" />
        <el-option label="Maintenance" value="maintenance" />
        <el-option label="Retired" value="retired" />
      </el-select>
      <el-input
        v-model="query.brand"
        placeholder="Brand"
        clearable
        style="width: 140px"
        @keyup.enter="loadVehicles"
        @clear="loadVehicles"
      />
      <el-button type="primary" @click="loadVehicles">
        <el-icon><Search /></el-icon> Search
      </el-button>
      <el-button v-if="authStore.isAdmin" type="primary" @click="$router.push('/vehicles/add')" style="margin-left: auto">
        <el-icon><Plus /></el-icon> Add Vehicle
      </el-button>
    </div>

    <!-- Vehicle Table -->
    <el-card>
      <el-table :data="vehicles" v-loading="loading" style="width: 100%" empty-text="No vehicles found">
        <el-table-column prop="plate_number" label="Plate No." min-width="120" />
        <el-table-column prop="brand" label="Brand" min-width="100" />
        <el-table-column prop="model" label="Model" min-width="100" />
        <el-table-column prop="color" label="Color" min-width="80" />
        <el-table-column prop="status" label="Status" min-width="110">
          <template #default="{ row }">
            <el-tag :type="vehicleStatusType(row.status)">{{ row.status }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="daily_rate" label="Daily Rate" min-width="100">
          <template #default="{ row }">
            ¥{{ row.daily_rate }}
          </template>
        </el-table-column>
        <el-table-column label="Actions" min-width="160" v-if="authStore.isAdmin">
          <template #default="{ row }">
            <el-button link type="primary" @click="$router.push(`/vehicles/${row.id}`)">View</el-button>
            <el-button link type="primary" @click="$router.push(`/vehicles/${row.id}/edit`)">Edit</el-button>
            <el-popconfirm title="Delete this vehicle?" @confirm="handleDelete(row.id)">
              <template #reference>
                <el-button link type="danger">Delete</el-button>
              </template>
            </el-popconfirm>
          </template>
        </el-table-column>
        <el-table-column label="Actions" min-width="80" v-else>
          <template #default="{ row }">
            <el-button link type="primary" @click="$router.push(`/vehicles/${row.id}`)">View</el-button>
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
import { useAuthStore } from '@/stores/auth'
import { getVehicleList, deleteVehicle } from '@/api/vehicle'
import { ElMessage } from 'element-plus'
import { Search, Plus } from '@element-plus/icons-vue'

const authStore = useAuthStore()
const loading = ref(false)
const vehicles = ref([])
const total = ref(0)

const query = reactive({
  page: 1,
  page_size: 20,
  keyword: '',
  status: '',
  brand: '',
})

function vehicleStatusType(status) {
  const map = {
    'available': 'success',
    'rented': 'warning',
    'maintenance': 'info',
    'retired': 'danger',
  }
  return map[(status || '').toLowerCase()] || 'info'
}

async function loadVehicles() {
  loading.value = true
  try {
    const params = { ...query }
    if (!params.keyword) delete params.keyword
    if (!params.status) delete params.status
    if (!params.brand) delete params.brand
    const res = await getVehicleList(params)
    if (res.error === 0) {
      vehicles.value = res.list || res.vehicles || []
      total.value = res.total || 0
    }
  } catch {
    ElMessage.error('Failed to load vehicles')
  } finally {
    loading.value = false
  }
}

async function handleDelete(id) {
  try {
    const res = await deleteVehicle({ id })
    if (res.error === 0) {
      ElMessage.success('Vehicle deleted')
      loadVehicles()
    } else {
      ElMessage.error('Failed to delete vehicle')
    }
  } catch {
    ElMessage.error('Failed to delete vehicle')
  }
}

function handlePageChange(page) {
  query.page = page
  loadVehicles()
}

onMounted(loadVehicles)
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
